#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_LIB_PATH "/mnt/data/lib/libmortox.so.0.0.0"
#define DEFAULT_SOCKET_PATH "/tmp/mortox.socket"
#define MORTOX_OPEN_UNIX 1
#define MORTOX_VALUE_CHAR 1
#define MORTOX_NAMESPACE "mem"
#define MORTOX_GROUP "default"
#define MORTOX_KEY_MOTOR_CMD "motor_cmd"
#define MOTOR_CMD_BUF_SIZE 32
#define MOTOR_SEQ_MIN 10
#define MOTOR_SEQ_MAX 99
#define MOTOR_DEVICE_ID 49

typedef void *(*libmortox_open_fn)(
    int type,
    const char *path,
    int port
);

typedef int (*libmortox_simple_set_fn)(
    void *client,
    const char *name_space,
    const char *group,
    const char *key,
    int value_type,
    const char *value
);

typedef int (*libmortox_simple_get_fn)(
    void *client,
    const char *name_space,
    const char *group,
    const char *key,
    void **value
);

typedef char *(*xdata_value_as_char_fn)(void *value);
typedef void (*xdata_free_fn)(void *value);
typedef void (*libmortox_close_fn)(void *client);

struct direction_spec {
	const char *name;
	int opcode;
};

static const struct direction_spec direction_specs[] = {
	{ "left",  1 },
	{ "right", 2 },
	{ "up",    3 },
	{ "down",  4 },
};

struct mortox_api {
	libmortox_open_fn libmortox_open;
	libmortox_simple_set_fn libmortox_simple_set;
	libmortox_simple_get_fn libmortox_simple_get;
	xdata_value_as_char_fn xdata_value_as_char;
	xdata_free_fn xdata_free;
	libmortox_close_fn libmortox_close;
};

static const struct direction_spec *find_direction(const char *direction)
{
	size_t i;

	for (i = 0; i < sizeof(direction_specs) / sizeof(direction_specs[0]); i++) {
		if (strcmp(direction, direction_specs[i].name) == 0) {
			return &direction_specs[i];
		}
	}

	return NULL;
}

static int clamp_motor_seq(int seq)
{
	if (seq < MOTOR_SEQ_MIN || seq > MOTOR_SEQ_MAX) {
		return MOTOR_SEQ_MIN;
	}

	return seq;
}

static int parse_motor_seq(const char *text, int *seq_out)
{
	int device_id;
	int opcode;
	int seq;

	if (!text || !seq_out) {
		return -1;
	}

	if (sscanf(text, "[%d,%d,%d]", &device_id, &opcode, &seq) != 3) {
		return -1;
	}

	if (device_id != MOTOR_DEVICE_ID) {
		return -1;
	}

	*seq_out = seq;
	return 0;
}

static int lookup_symbol(void *handle, const char *name, void **symbol_out)
{
	dlerror();
	*symbol_out = dlsym(handle, name);

	if (!*symbol_out) {
		fprintf(stderr, "dlsym(%s) failed: %s\n", name, dlerror());
		return -1;
	}

	return 0;
}

static int load_mortox_api(void *handle, struct mortox_api *api)
{
	if (lookup_symbol(handle, "libmortox_open", (void **)&api->libmortox_open) < 0 ||
	    lookup_symbol(handle, "libmortox_simple_set", (void **)&api->libmortox_simple_set) < 0 ||
	    lookup_symbol(handle, "libmortox_simple_get", (void **)&api->libmortox_simple_get) < 0 ||
	    lookup_symbol(handle, "xdata_value_as_char", (void **)&api->xdata_value_as_char) < 0 ||
	    lookup_symbol(handle, "xdata_free", (void **)&api->xdata_free) < 0 ||
	    lookup_symbol(handle, "libmortox_close", (void **)&api->libmortox_close) < 0) {
		return -1;
	}

	return 0;
}

static int next_motor_seq(
    const struct mortox_api *api,
    void *client,
    char *current_cmd,
    size_t current_cmd_size
)
{
	void *value = NULL;
	char *text = NULL;
	int c = 52;
	int ret;

	if (current_cmd && current_cmd_size > 0) {
		current_cmd[0] = '\0';
	}

	ret = api->libmortox_simple_get(client, MORTOX_NAMESPACE, MORTOX_GROUP, MORTOX_KEY_MOTOR_CMD, &value);

	if (ret == 1 && value) {
		text = api->xdata_value_as_char(value);

		if (text) {
			if (current_cmd && current_cmd_size > 0) {
				snprintf(current_cmd, current_cmd_size, "%s", text);
			}

			if (parse_motor_seq(text, &c) < 0) {
				c = 52;
			}
		}

		api->xdata_free(value);
	}

	return clamp_motor_seq(c + 1);
}

static int build_motor_cmd(const struct direction_spec *direction, int seq, char *buf, size_t buf_size)
{
	int written;

	if (!direction || !buf || buf_size == 0) {
		return -1;
	}

	written = snprintf(buf, buf_size, "[%d,%d,%d]", MOTOR_DEVICE_ID, direction->opcode, clamp_motor_seq(seq));

	if (written < 0 || (size_t)written >= buf_size) {
		return -1;
	}

	return 0;
}

static void usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [left|right|up|down] [libmortox_path] [socket_path]\n", argv0);
}

int main(int argc, char **argv)
{
	const char *lib_path = DEFAULT_LIB_PATH;
	const char *socket_path = DEFAULT_SOCKET_PATH;
	const struct direction_spec *direction;
	void *handle;
	struct mortox_api api;
	void *client;
	char motor_cmd[MOTOR_CMD_BUF_SIZE];
	char current_cmd[MOTOR_CMD_BUF_SIZE];
	int seq;
	int ret;

	if (argc < 2 || argc > 4) {
		usage(argv[0]);
		return 1;
	}

	direction = find_direction(argv[1]);

	if (!direction) {
		usage(argv[0]);
		return 1;
	}

	if (argc == 3) {
		lib_path = argv[2];
	}

	if (argc == 4) {
		socket_path = argv[3];
	}

	handle = dlopen(lib_path, RTLD_NOW);

	if (!handle) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 1;
	}

	memset(&api, 0, sizeof(api));

	if (load_mortox_api(handle, &api) < 0) {
		dlclose(handle);
		return 1;
	}

	client = api.libmortox_open(MORTOX_OPEN_UNIX, socket_path, 0);

	if (!client) {
		fprintf(stderr, "libmortox_open(%s) failed: %s\n", socket_path, strerror(errno));
		dlclose(handle);
		return 2;
	}

	seq = next_motor_seq(&api, client, current_cmd, sizeof(current_cmd));

	if (build_motor_cmd(direction, seq, motor_cmd, sizeof(motor_cmd)) < 0) {
		api.libmortox_close(client);
		dlclose(handle);
		fprintf(stderr, "build motor command failed\n");
		return 1;
	}

	ret = api.libmortox_simple_set(client, MORTOX_NAMESPACE, MORTOX_GROUP, MORTOX_KEY_MOTOR_CMD,
	                               MORTOX_VALUE_CHAR, motor_cmd);
	printf("current=%s next=%s seq=%d result=%d\n",
	       current_cmd[0] ? current_cmd : "(empty)", motor_cmd, seq, ret);

	api.libmortox_close(client);

	dlclose(handle);
	return ret ? 0 : 2;
}
