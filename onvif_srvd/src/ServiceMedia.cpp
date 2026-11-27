/*
 --------------------------------------------------------------------------
 ServiceMedia.cpp

 Implementation of functions (methods) for the service:
 ONVIF media.wsdl server side
-----------------------------------------------------------------------------
*/


#include "soapMediaBindingService.h"
#include "ServiceContext.h"
#include "smacros.h"
#include "stools.h"





int MediaBindingService::GetServiceCapabilities(_trt__GetServiceCapabilities *trt__GetServiceCapabilities, _trt__GetServiceCapabilitiesResponse &trt__GetServiceCapabilitiesResponse)
{
    UNUSED(trt__GetServiceCapabilities);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);


    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetServiceCapabilitiesResponse.Capabilities = ctx->getMediaServiceCapabilities(this->soap);


    return SOAP_OK;
}



int MediaBindingService::GetVideoSources(_trt__GetVideoSources *trt__GetVideoSources, _trt__GetVideoSourcesResponse &trt__GetVideoSourcesResponse)
{
    UNUSED(trt__GetVideoSources);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);


    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    auto profiles = ctx->get_profiles();


    for( auto it = profiles.cbegin(); it != profiles.cend(); ++it )
    {
        trt__GetVideoSourcesResponse.VideoSources.push_back(it->second.get_video_src(this->soap));
    }


    return SOAP_OK;
}



int MediaBindingService::GetAudioSources(_trt__GetAudioSources *trt__GetAudioSources, _trt__GetAudioSourcesResponse &trt__GetAudioSourcesResponse)
{
    UNUSED(trt__GetAudioSources);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioSourcesResponse.AudioSources.push_back(ctx->get_audio_src(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetAudioOutputs(_trt__GetAudioOutputs *trt__GetAudioOutputs, _trt__GetAudioOutputsResponse &trt__GetAudioOutputsResponse)
{
    UNUSED(trt__GetAudioOutputs);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioOutputsResponse.AudioOutputs.push_back(ctx->get_audio_out(this->soap));
    return SOAP_OK;
}



int MediaBindingService::CreateProfile(_trt__CreateProfile *trt__CreateProfile, _trt__CreateProfileResponse &trt__CreateProfileResponse)
{
    SOAP_EMPTY_HANDLER(trt__CreateProfile, "Media");
}



int MediaBindingService::GetProfile(_trt__GetProfile *trt__GetProfile, _trt__GetProfileResponse &trt__GetProfileResponse)
{
    DEBUG_MSG("Media: %s   get profile:%s\n", __FUNCTION__, trt__GetProfile->ProfileToken.c_str());

    int ret = SOAP_FAULT;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles       = ctx->get_profiles();
    auto it             = profiles.find(trt__GetProfile->ProfileToken);


    if( it != profiles.end() )
    {
        trt__GetProfileResponse.Profile = it->second.get_profile(this->soap);
        ret = SOAP_OK;
    }


    return ret;
}



int MediaBindingService::GetProfiles(_trt__GetProfiles *trt__GetProfiles, _trt__GetProfilesResponse &trt__GetProfilesResponse)
{
    UNUSED(trt__GetProfiles);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);


    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    auto profiles = ctx->get_profiles();


    for( auto it = profiles.cbegin(); it != profiles.cend(); ++it )
    {
        trt__GetProfilesResponse.Profiles.push_back(it->second.get_profile(this->soap));
    }


    return SOAP_OK;
}



int MediaBindingService::AddVideoEncoderConfiguration(_trt__AddVideoEncoderConfiguration *trt__AddVideoEncoderConfiguration, _trt__AddVideoEncoderConfigurationResponse &trt__AddVideoEncoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddVideoEncoderConfiguration, "Media");
}



int MediaBindingService::AddVideoSourceConfiguration(_trt__AddVideoSourceConfiguration *trt__AddVideoSourceConfiguration, _trt__AddVideoSourceConfigurationResponse &trt__AddVideoSourceConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddVideoSourceConfiguration, "Media");
}



int MediaBindingService::AddAudioEncoderConfiguration(_trt__AddAudioEncoderConfiguration *trt__AddAudioEncoderConfiguration, _trt__AddAudioEncoderConfigurationResponse &trt__AddAudioEncoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddAudioEncoderConfiguration, "Media");
}



int MediaBindingService::AddAudioSourceConfiguration(_trt__AddAudioSourceConfiguration *trt__AddAudioSourceConfiguration, _trt__AddAudioSourceConfigurationResponse &trt__AddAudioSourceConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddAudioSourceConfiguration, "Media");
}



int MediaBindingService::AddPTZConfiguration(_trt__AddPTZConfiguration *trt__AddPTZConfiguration, _trt__AddPTZConfigurationResponse &trt__AddPTZConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddPTZConfiguration, "Media");
}



int MediaBindingService::AddVideoAnalyticsConfiguration(_trt__AddVideoAnalyticsConfiguration *trt__AddVideoAnalyticsConfiguration, _trt__AddVideoAnalyticsConfigurationResponse &trt__AddVideoAnalyticsConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddVideoAnalyticsConfiguration, "Media");
}



int MediaBindingService::AddMetadataConfiguration(_trt__AddMetadataConfiguration *trt__AddMetadataConfiguration, _trt__AddMetadataConfigurationResponse &trt__AddMetadataConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddMetadataConfiguration, "Media");
}



int MediaBindingService::AddAudioOutputConfiguration(_trt__AddAudioOutputConfiguration *trt__AddAudioOutputConfiguration, _trt__AddAudioOutputConfigurationResponse &trt__AddAudioOutputConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddAudioOutputConfiguration, "Media");
}



int MediaBindingService::AddAudioDecoderConfiguration(_trt__AddAudioDecoderConfiguration *trt__AddAudioDecoderConfiguration, _trt__AddAudioDecoderConfigurationResponse &trt__AddAudioDecoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__AddAudioDecoderConfiguration, "Media");
}



int MediaBindingService::RemoveVideoEncoderConfiguration(_trt__RemoveVideoEncoderConfiguration *trt__RemoveVideoEncoderConfiguration, _trt__RemoveVideoEncoderConfigurationResponse &trt__RemoveVideoEncoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveVideoEncoderConfiguration, "Media");
}



int MediaBindingService::RemoveVideoSourceConfiguration(_trt__RemoveVideoSourceConfiguration *trt__RemoveVideoSourceConfiguration, _trt__RemoveVideoSourceConfigurationResponse &trt__RemoveVideoSourceConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveVideoSourceConfiguration, "Media");
}



int MediaBindingService::RemoveAudioEncoderConfiguration(_trt__RemoveAudioEncoderConfiguration *trt__RemoveAudioEncoderConfiguration, _trt__RemoveAudioEncoderConfigurationResponse &trt__RemoveAudioEncoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveAudioEncoderConfiguration, "Media");
}



int MediaBindingService::RemoveAudioSourceConfiguration(_trt__RemoveAudioSourceConfiguration *trt__RemoveAudioSourceConfiguration, _trt__RemoveAudioSourceConfigurationResponse &trt__RemoveAudioSourceConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveAudioSourceConfiguration, "Media");
}



int MediaBindingService::RemovePTZConfiguration(_trt__RemovePTZConfiguration *trt__RemovePTZConfiguration, _trt__RemovePTZConfigurationResponse &trt__RemovePTZConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemovePTZConfiguration, "Media");
}



int MediaBindingService::RemoveVideoAnalyticsConfiguration(_trt__RemoveVideoAnalyticsConfiguration *trt__RemoveVideoAnalyticsConfiguration, _trt__RemoveVideoAnalyticsConfigurationResponse &trt__RemoveVideoAnalyticsConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveVideoAnalyticsConfiguration, "Media");
}



int MediaBindingService::RemoveMetadataConfiguration(_trt__RemoveMetadataConfiguration *trt__RemoveMetadataConfiguration, _trt__RemoveMetadataConfigurationResponse &trt__RemoveMetadataConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveMetadataConfiguration, "Media");
}



int MediaBindingService::RemoveAudioOutputConfiguration(_trt__RemoveAudioOutputConfiguration *trt__RemoveAudioOutputConfiguration, _trt__RemoveAudioOutputConfigurationResponse &trt__RemoveAudioOutputConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveAudioOutputConfiguration, "Media");
}



int MediaBindingService::RemoveAudioDecoderConfiguration(_trt__RemoveAudioDecoderConfiguration *trt__RemoveAudioDecoderConfiguration, _trt__RemoveAudioDecoderConfigurationResponse &trt__RemoveAudioDecoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__RemoveAudioDecoderConfiguration, "Media");
}



int MediaBindingService::DeleteProfile(_trt__DeleteProfile *trt__DeleteProfile, _trt__DeleteProfileResponse &trt__DeleteProfileResponse)
{
    SOAP_EMPTY_HANDLER(trt__DeleteProfile, "Media");
}



int MediaBindingService::GetVideoSourceConfigurations(_trt__GetVideoSourceConfigurations *trt__GetVideoSourceConfigurations, _trt__GetVideoSourceConfigurationsResponse &trt__GetVideoSourceConfigurationsResponse)
{
    UNUSED(trt__GetVideoSourceConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    for (auto it = profiles.cbegin(); it != profiles.cend(); ++it) {
        trt__GetVideoSourceConfigurationsResponse.Configurations.push_back(it->second.get_video_src_cnf(this->soap));
    }
    return SOAP_OK;
}



int MediaBindingService::GetVideoEncoderConfigurations(_trt__GetVideoEncoderConfigurations *trt__GetVideoEncoderConfigurations, _trt__GetVideoEncoderConfigurationsResponse &trt__GetVideoEncoderConfigurationsResponse)
{
    UNUSED(trt__GetVideoEncoderConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    for (auto it = profiles.cbegin(); it != profiles.cend(); ++it) {
        trt__GetVideoEncoderConfigurationsResponse.Configurations.push_back(it->second.get_video_enc_cfg(this->soap));
    }
    return SOAP_OK;
}



int MediaBindingService::GetAudioSourceConfigurations(_trt__GetAudioSourceConfigurations *trt__GetAudioSourceConfigurations, _trt__GetAudioSourceConfigurationsResponse &trt__GetAudioSourceConfigurationsResponse)
{
    UNUSED(trt__GetAudioSourceConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioSourceConfigurationsResponse.Configurations.push_back(ctx->get_audio_src_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetAudioEncoderConfigurations(_trt__GetAudioEncoderConfigurations *trt__GetAudioEncoderConfigurations, _trt__GetAudioEncoderConfigurationsResponse &trt__GetAudioEncoderConfigurationsResponse)
{
    UNUSED(trt__GetAudioEncoderConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioEncoderConfigurationsResponse.Configurations.push_back(ctx->get_audio_enc_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetVideoAnalyticsConfigurations(_trt__GetVideoAnalyticsConfigurations *trt__GetVideoAnalyticsConfigurations, _trt__GetVideoAnalyticsConfigurationsResponse &trt__GetVideoAnalyticsConfigurationsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetVideoAnalyticsConfigurations, "Media");
}



int MediaBindingService::GetMetadataConfigurations(_trt__GetMetadataConfigurations *trt__GetMetadataConfigurations, _trt__GetMetadataConfigurationsResponse &trt__GetMetadataConfigurationsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetMetadataConfigurations, "Media");
}



int MediaBindingService::GetAudioOutputConfigurations(_trt__GetAudioOutputConfigurations *trt__GetAudioOutputConfigurations, _trt__GetAudioOutputConfigurationsResponse &trt__GetAudioOutputConfigurationsResponse)
{
    UNUSED(trt__GetAudioOutputConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioOutputConfigurationsResponse.Configurations.push_back(ctx->get_audio_out_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetAudioDecoderConfigurations(_trt__GetAudioDecoderConfigurations *trt__GetAudioDecoderConfigurations, _trt__GetAudioDecoderConfigurationsResponse &trt__GetAudioDecoderConfigurationsResponse)
{
    UNUSED(trt__GetAudioDecoderConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioDecoderConfigurationsResponse.Configurations.push_back(ctx->get_audio_dec_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetVideoSourceConfiguration(_trt__GetVideoSourceConfiguration *trt__GetVideoSourceConfiguration, _trt__GetVideoSourceConfigurationResponse &trt__GetVideoSourceConfigurationResponse)
{
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    auto it = profiles.find(trt__GetVideoSourceConfiguration->ConfigurationToken);
    if (it == profiles.end()) {
        it = profiles.begin();
    }
    if (it != profiles.end()) {
        trt__GetVideoSourceConfigurationResponse.Configuration = it->second.get_video_src_cnf(this->soap);
        return SOAP_OK;
    }
    return SOAP_FAULT;
}



int MediaBindingService::GetVideoEncoderConfiguration(_trt__GetVideoEncoderConfiguration *trt__GetVideoEncoderConfiguration, _trt__GetVideoEncoderConfigurationResponse &trt__GetVideoEncoderConfigurationResponse)
{
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    auto it = profiles.find(trt__GetVideoEncoderConfiguration->ConfigurationToken);
    if (it == profiles.end()) {
        it = profiles.begin();
    }
    if (it != profiles.end()) {
        trt__GetVideoEncoderConfigurationResponse.Configuration = it->second.get_video_enc_cfg(this->soap);
        return SOAP_OK;
    }
    return SOAP_FAULT;
}



int MediaBindingService::GetAudioSourceConfiguration(_trt__GetAudioSourceConfiguration *trt__GetAudioSourceConfiguration, _trt__GetAudioSourceConfigurationResponse &trt__GetAudioSourceConfigurationResponse)
{
    UNUSED(trt__GetAudioSourceConfiguration);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioSourceConfigurationResponse.Configuration = ctx->get_audio_src_cfg(this->soap);
    return SOAP_OK;
}



int MediaBindingService::GetAudioEncoderConfiguration(_trt__GetAudioEncoderConfiguration *trt__GetAudioEncoderConfiguration, _trt__GetAudioEncoderConfigurationResponse &trt__GetAudioEncoderConfigurationResponse)
{
    UNUSED(trt__GetAudioEncoderConfiguration);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioEncoderConfigurationResponse.Configuration = ctx->get_audio_enc_cfg(this->soap);
    return SOAP_OK;
}



int MediaBindingService::GetVideoAnalyticsConfiguration(_trt__GetVideoAnalyticsConfiguration *trt__GetVideoAnalyticsConfiguration, _trt__GetVideoAnalyticsConfigurationResponse &trt__GetVideoAnalyticsConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetVideoAnalyticsConfiguration, "Media");
}



int MediaBindingService::GetMetadataConfiguration(_trt__GetMetadataConfiguration *trt__GetMetadataConfiguration, _trt__GetMetadataConfigurationResponse &trt__GetMetadataConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetMetadataConfiguration, "Media");
}



int MediaBindingService::GetAudioOutputConfiguration(_trt__GetAudioOutputConfiguration *trt__GetAudioOutputConfiguration, _trt__GetAudioOutputConfigurationResponse &trt__GetAudioOutputConfigurationResponse)
{
    UNUSED(trt__GetAudioOutputConfiguration);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioOutputConfigurationResponse.Configuration = ctx->get_audio_out_cfg(this->soap);
    return SOAP_OK;
}



int MediaBindingService::GetAudioDecoderConfiguration(_trt__GetAudioDecoderConfiguration *trt__GetAudioDecoderConfiguration, _trt__GetAudioDecoderConfigurationResponse &trt__GetAudioDecoderConfigurationResponse)
{
    UNUSED(trt__GetAudioDecoderConfiguration);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetAudioDecoderConfigurationResponse.Configuration = ctx->get_audio_dec_cfg(this->soap);
    return SOAP_OK;
}



int MediaBindingService::GetCompatibleVideoEncoderConfigurations(_trt__GetCompatibleVideoEncoderConfigurations *trt__GetCompatibleVideoEncoderConfigurations, _trt__GetCompatibleVideoEncoderConfigurationsResponse &trt__GetCompatibleVideoEncoderConfigurationsResponse)
{
    UNUSED(trt__GetCompatibleVideoEncoderConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    for (auto it = profiles.cbegin(); it != profiles.cend(); ++it) {
        trt__GetCompatibleVideoEncoderConfigurationsResponse.Configurations.push_back(it->second.get_video_enc_cfg(this->soap));
    }
    return SOAP_OK;
}



int MediaBindingService::GetCompatibleVideoSourceConfigurations(_trt__GetCompatibleVideoSourceConfigurations *trt__GetCompatibleVideoSourceConfigurations, _trt__GetCompatibleVideoSourceConfigurationsResponse &trt__GetCompatibleVideoSourceConfigurationsResponse)
{
    UNUSED(trt__GetCompatibleVideoSourceConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    for (auto it = profiles.cbegin(); it != profiles.cend(); ++it) {
        trt__GetCompatibleVideoSourceConfigurationsResponse.Configurations.push_back(it->second.get_video_src_cnf(this->soap));
    }
    return SOAP_OK;
}



int MediaBindingService::GetCompatibleAudioEncoderConfigurations(_trt__GetCompatibleAudioEncoderConfigurations *trt__GetCompatibleAudioEncoderConfigurations, _trt__GetCompatibleAudioEncoderConfigurationsResponse &trt__GetCompatibleAudioEncoderConfigurationsResponse)
{
    UNUSED(trt__GetCompatibleAudioEncoderConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetCompatibleAudioEncoderConfigurationsResponse.Configurations.push_back(ctx->get_audio_enc_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetCompatibleAudioSourceConfigurations(_trt__GetCompatibleAudioSourceConfigurations *trt__GetCompatibleAudioSourceConfigurations, _trt__GetCompatibleAudioSourceConfigurationsResponse &trt__GetCompatibleAudioSourceConfigurationsResponse)
{
    UNUSED(trt__GetCompatibleAudioSourceConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetCompatibleAudioSourceConfigurationsResponse.Configurations.push_back(ctx->get_audio_src_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetCompatibleVideoAnalyticsConfigurations(_trt__GetCompatibleVideoAnalyticsConfigurations *trt__GetCompatibleVideoAnalyticsConfigurations, _trt__GetCompatibleVideoAnalyticsConfigurationsResponse &trt__GetCompatibleVideoAnalyticsConfigurationsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetCompatibleVideoAnalyticsConfigurations, "Media");
}



int MediaBindingService::GetCompatibleMetadataConfigurations(_trt__GetCompatibleMetadataConfigurations *trt__GetCompatibleMetadataConfigurations, _trt__GetCompatibleMetadataConfigurationsResponse &trt__GetCompatibleMetadataConfigurationsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetCompatibleMetadataConfigurations, "Media");
}



int MediaBindingService::GetCompatibleAudioOutputConfigurations(_trt__GetCompatibleAudioOutputConfigurations *trt__GetCompatibleAudioOutputConfigurations, _trt__GetCompatibleAudioOutputConfigurationsResponse &trt__GetCompatibleAudioOutputConfigurationsResponse)
{
    UNUSED(trt__GetCompatibleAudioOutputConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetCompatibleAudioOutputConfigurationsResponse.Configurations.push_back(ctx->get_audio_out_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::GetCompatibleAudioDecoderConfigurations(_trt__GetCompatibleAudioDecoderConfigurations *trt__GetCompatibleAudioDecoderConfigurations, _trt__GetCompatibleAudioDecoderConfigurationsResponse &trt__GetCompatibleAudioDecoderConfigurationsResponse)
{
    UNUSED(trt__GetCompatibleAudioDecoderConfigurations);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    trt__GetCompatibleAudioDecoderConfigurationsResponse.Configurations.push_back(ctx->get_audio_dec_cfg(this->soap));
    return SOAP_OK;
}



int MediaBindingService::SetVideoSourceConfiguration(_trt__SetVideoSourceConfiguration *trt__SetVideoSourceConfiguration, _trt__SetVideoSourceConfigurationResponse &trt__SetVideoSourceConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetVideoSourceConfiguration, "Media");
}



int MediaBindingService::SetVideoEncoderConfiguration(_trt__SetVideoEncoderConfiguration *trt__SetVideoEncoderConfiguration, _trt__SetVideoEncoderConfigurationResponse &trt__SetVideoEncoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetVideoEncoderConfiguration, "Media");
}



int MediaBindingService::SetAudioSourceConfiguration(_trt__SetAudioSourceConfiguration *trt__SetAudioSourceConfiguration, _trt__SetAudioSourceConfigurationResponse &trt__SetAudioSourceConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetAudioSourceConfiguration, "Media");
}



int MediaBindingService::SetAudioEncoderConfiguration(_trt__SetAudioEncoderConfiguration *trt__SetAudioEncoderConfiguration, _trt__SetAudioEncoderConfigurationResponse &trt__SetAudioEncoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetAudioEncoderConfiguration, "Media");
}



int MediaBindingService::SetVideoAnalyticsConfiguration(_trt__SetVideoAnalyticsConfiguration *trt__SetVideoAnalyticsConfiguration, _trt__SetVideoAnalyticsConfigurationResponse &trt__SetVideoAnalyticsConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetVideoAnalyticsConfiguration, "Media");
}



int MediaBindingService::SetMetadataConfiguration(_trt__SetMetadataConfiguration *trt__SetMetadataConfiguration, _trt__SetMetadataConfigurationResponse &trt__SetMetadataConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetMetadataConfiguration, "Media");
}



int MediaBindingService::SetAudioOutputConfiguration(_trt__SetAudioOutputConfiguration *trt__SetAudioOutputConfiguration, _trt__SetAudioOutputConfigurationResponse &trt__SetAudioOutputConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetAudioOutputConfiguration, "Media");
}



int MediaBindingService::SetAudioDecoderConfiguration(_trt__SetAudioDecoderConfiguration *trt__SetAudioDecoderConfiguration, _trt__SetAudioDecoderConfigurationResponse &trt__SetAudioDecoderConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetAudioDecoderConfiguration, "Media");
}



int MediaBindingService::GetVideoSourceConfigurationOptions(_trt__GetVideoSourceConfigurationOptions *trt__GetVideoSourceConfigurationOptions, _trt__GetVideoSourceConfigurationOptionsResponse &trt__GetVideoSourceConfigurationOptionsResponse)
{
    UNUSED(trt__GetVideoSourceConfigurationOptions);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    int max_w = 2304, max_h = 1296;
    tt__VideoSourceConfigurationOptions *opts = soap_new_tt__VideoSourceConfigurationOptions(this->soap);
    for (auto it = profiles.cbegin(); it != profiles.cend(); ++it) {
        opts->VideoSourceTokensAvailable.push_back(it->second.get_name());
        if (it->second.get_width() > max_w) max_w = it->second.get_width();
        if (it->second.get_height() > max_h) max_h = it->second.get_height();
    }
    opts->BoundsRange = soap_new_tt__IntRectangleRange(this->soap);
    opts->BoundsRange->XRange      = soap_new_req_tt__IntRange(this->soap, 0, 0);
    opts->BoundsRange->YRange      = soap_new_req_tt__IntRange(this->soap, 0, 0);
    opts->BoundsRange->WidthRange  = soap_new_req_tt__IntRange(this->soap, 640, max_w);
    opts->BoundsRange->HeightRange = soap_new_req_tt__IntRange(this->soap, 360, max_h);
    trt__GetVideoSourceConfigurationOptionsResponse.Options = opts;
    return SOAP_OK;
}



int MediaBindingService::GetVideoEncoderConfigurationOptions(_trt__GetVideoEncoderConfigurationOptions *trt__GetVideoEncoderConfigurationOptions, _trt__GetVideoEncoderConfigurationOptionsResponse &trt__GetVideoEncoderConfigurationOptionsResponse)
{
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles = ctx->get_profiles();
    tt__VideoEncoderConfigurationOptions *opts = soap_new_tt__VideoEncoderConfigurationOptions(this->soap);
    opts->QualityRange = soap_new_req_tt__IntRange(this->soap, 1, 10);

    opts->H264 = soap_new_tt__H264Options(this->soap);
    if (trt__GetVideoEncoderConfigurationOptions &&
        trt__GetVideoEncoderConfigurationOptions->ConfigurationToken &&
        profiles.find(*trt__GetVideoEncoderConfigurationOptions->ConfigurationToken) != profiles.end()) {
        const auto &prof = profiles.at(*trt__GetVideoEncoderConfigurationOptions->ConfigurationToken);
        opts->H264->ResolutionsAvailable.push_back(soap_new_req_tt__VideoResolution(this->soap, prof.get_width(), prof.get_height()));
    } else if (trt__GetVideoEncoderConfigurationOptions &&
               trt__GetVideoEncoderConfigurationOptions->ProfileToken &&
               profiles.find(*trt__GetVideoEncoderConfigurationOptions->ProfileToken) != profiles.end()) {
        const auto &prof = profiles.at(*trt__GetVideoEncoderConfigurationOptions->ProfileToken);
        opts->H264->ResolutionsAvailable.push_back(soap_new_req_tt__VideoResolution(this->soap, prof.get_width(), prof.get_height()));
    } else {
        for (auto it = profiles.cbegin(); it != profiles.cend(); ++it) {
            opts->H264->ResolutionsAvailable.push_back(soap_new_req_tt__VideoResolution(this->soap, it->second.get_width(), it->second.get_height()));
        }
    }
    opts->H264->GovLengthRange        = soap_new_req_tt__IntRange(this->soap, 1, 100);
    opts->H264->FrameRateRange        = soap_new_req_tt__IntRange(this->soap, 1, 25);
    opts->H264->EncodingIntervalRange = soap_new_req_tt__IntRange(this->soap, 1, 1);
    opts->H264->H264ProfilesSupported.push_back(tt__H264Profile__Baseline);
    opts->H264->H264ProfilesSupported.push_back(tt__H264Profile__Main);
    opts->H264->H264ProfilesSupported.push_back(tt__H264Profile__High);

    trt__GetVideoEncoderConfigurationOptionsResponse.Options = opts;
    return SOAP_OK;
}



int MediaBindingService::GetAudioSourceConfigurationOptions(_trt__GetAudioSourceConfigurationOptions *trt__GetAudioSourceConfigurationOptions, _trt__GetAudioSourceConfigurationOptionsResponse &trt__GetAudioSourceConfigurationOptionsResponse)
{
    UNUSED(trt__GetAudioSourceConfigurationOptions);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    tt__AudioSourceConfigurationOptions *opts = soap_new_tt__AudioSourceConfigurationOptions(this->soap);
    opts->InputTokensAvailable.push_back("AudioSource0");
    trt__GetAudioSourceConfigurationOptionsResponse.Options = opts;
    return SOAP_OK;
}



int MediaBindingService::GetAudioEncoderConfigurationOptions(_trt__GetAudioEncoderConfigurationOptions *trt__GetAudioEncoderConfigurationOptions, _trt__GetAudioEncoderConfigurationOptionsResponse &trt__GetAudioEncoderConfigurationOptionsResponse)
{
    UNUSED(trt__GetAudioEncoderConfigurationOptions);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    tt__AudioEncoderConfigurationOptions *opts = soap_new_tt__AudioEncoderConfigurationOptions(this->soap);
    tt__AudioEncoderConfigurationOption *opt = soap_new_tt__AudioEncoderConfigurationOption(this->soap);
    opt->Encoding = tt__AudioEncoding__G711;
    opt->BitrateList = soap_new_tt__IntList(this->soap);
    opt->BitrateList->Items.push_back(64);
    opt->SampleRateList = soap_new_tt__IntList(this->soap);
    opt->SampleRateList->Items.push_back(8);
    opts->Options.push_back(opt);
    trt__GetAudioEncoderConfigurationOptionsResponse.Options = opts;
    return SOAP_OK;
}



int MediaBindingService::GetMetadataConfigurationOptions(_trt__GetMetadataConfigurationOptions *trt__GetMetadataConfigurationOptions, _trt__GetMetadataConfigurationOptionsResponse &trt__GetMetadataConfigurationOptionsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetMetadataConfigurationOptions, "Media");
}



int MediaBindingService::GetAudioOutputConfigurationOptions(_trt__GetAudioOutputConfigurationOptions *trt__GetAudioOutputConfigurationOptions, _trt__GetAudioOutputConfigurationOptionsResponse &trt__GetAudioOutputConfigurationOptionsResponse)
{
    UNUSED(trt__GetAudioOutputConfigurationOptions);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    tt__AudioOutputConfigurationOptions *opts = soap_new_tt__AudioOutputConfigurationOptions(this->soap);
    opts->OutputTokensAvailable.push_back("AudioOutput0");
    opts->SendPrimacyOptions.push_back("www.onvif.org/ver20/HalfDuplex/Auto");
    opts->OutputLevelRange = soap_new_req_tt__IntRange(this->soap, 0, 100);
    trt__GetAudioOutputConfigurationOptionsResponse.Options = opts;
    return SOAP_OK;
}



int MediaBindingService::GetAudioDecoderConfigurationOptions(_trt__GetAudioDecoderConfigurationOptions *trt__GetAudioDecoderConfigurationOptions, _trt__GetAudioDecoderConfigurationOptionsResponse &trt__GetAudioDecoderConfigurationOptionsResponse)
{
    UNUSED(trt__GetAudioDecoderConfigurationOptions);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    tt__AudioDecoderConfigurationOptions *opts = soap_new_tt__AudioDecoderConfigurationOptions(this->soap);
    opts->G711DecOptions = soap_new_tt__G711DecOptions(this->soap);
    opts->G711DecOptions->Bitrate = soap_new_tt__IntList(this->soap);
    opts->G711DecOptions->Bitrate->Items.push_back(64);
    opts->G711DecOptions->SampleRateRange = soap_new_tt__IntList(this->soap);
    opts->G711DecOptions->SampleRateRange->Items.push_back(8);
    trt__GetAudioDecoderConfigurationOptionsResponse.Options = opts;
    return SOAP_OK;
}



int MediaBindingService::GetGuaranteedNumberOfVideoEncoderInstances(_trt__GetGuaranteedNumberOfVideoEncoderInstances *trt__GetGuaranteedNumberOfVideoEncoderInstances, _trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse &trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse)
{
    UNUSED(trt__GetGuaranteedNumberOfVideoEncoderInstances);
    DEBUG_MSG("Media: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    int cnt = (int)ctx->get_profiles().size();
    trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse.TotalNumber = cnt;
    trt__GetGuaranteedNumberOfVideoEncoderInstancesResponse.H264        = soap_new_ptr(this->soap, cnt);
    return SOAP_OK;
}



int MediaBindingService::GetStreamUri(_trt__GetStreamUri *trt__GetStreamUri, _trt__GetStreamUriResponse &trt__GetStreamUriResponse)
{
    DEBUG_MSG("Media: %s   for profile:%s\n", __FUNCTION__, trt__GetStreamUri->ProfileToken.c_str());


    int ret = SOAP_FAULT;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles       = ctx->get_profiles();
    auto it             = profiles.find(trt__GetStreamUri->ProfileToken);


    if( it != profiles.end() )
    {
        trt__GetStreamUriResponse.MediaUri = soap_new_tt__MediaUri(this->soap);
        trt__GetStreamUriResponse.MediaUri->Uri = ctx->get_stream_uri(it->second.get_url(), htonl(this->soap->ip));
        ret = SOAP_OK;
    }


    return ret;
}



int MediaBindingService::StartMulticastStreaming(_trt__StartMulticastStreaming *trt__StartMulticastStreaming, _trt__StartMulticastStreamingResponse &trt__StartMulticastStreamingResponse)
{
    SOAP_EMPTY_HANDLER(trt__StartMulticastStreaming, "Media");
}



int MediaBindingService::StopMulticastStreaming(_trt__StopMulticastStreaming *trt__StopMulticastStreaming, _trt__StopMulticastStreamingResponse &trt__StopMulticastStreamingResponse)
{
    SOAP_EMPTY_HANDLER(trt__StopMulticastStreaming, "Media");
}



int MediaBindingService::SetSynchronizationPoint(_trt__SetSynchronizationPoint *trt__SetSynchronizationPoint, _trt__SetSynchronizationPointResponse &trt__SetSynchronizationPointResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetSynchronizationPoint, "Media");
}



int MediaBindingService::GetSnapshotUri(_trt__GetSnapshotUri *trt__GetSnapshotUri, _trt__GetSnapshotUriResponse &trt__GetSnapshotUriResponse)
{
    DEBUG_MSG("Media: %s   for profile:%s\n", __FUNCTION__, trt__GetSnapshotUri->ProfileToken.c_str());


    int ret = SOAP_FAULT;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    auto profiles       = ctx->get_profiles();
    auto it             = profiles.find(trt__GetSnapshotUri->ProfileToken);

    if( it != profiles.end() )
    {
        trt__GetSnapshotUriResponse.MediaUri = soap_new_tt__MediaUri(this->soap);
        trt__GetSnapshotUriResponse.MediaUri->Uri = ctx->get_snapshot_uri(it->second.get_snapurl(), htonl(this->soap->ip));
        ret = SOAP_OK;
    }


    return ret;
}



int MediaBindingService::GetVideoSourceModes(_trt__GetVideoSourceModes *trt__GetVideoSourceModes, _trt__GetVideoSourceModesResponse &trt__GetVideoSourceModesResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetVideoSourceModes, "Media");
}



int MediaBindingService::SetVideoSourceMode(_trt__SetVideoSourceMode *trt__SetVideoSourceMode, _trt__SetVideoSourceModeResponse &trt__SetVideoSourceModeResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetVideoSourceMode, "Media");
}



int MediaBindingService::GetOSDs(_trt__GetOSDs *trt__GetOSDs, _trt__GetOSDsResponse &trt__GetOSDsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetOSDs, "Media");
}



int MediaBindingService::GetOSD(_trt__GetOSD *trt__GetOSD, _trt__GetOSDResponse &trt__GetOSDResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetOSD, "Media");
}



int MediaBindingService::GetOSDOptions(_trt__GetOSDOptions *trt__GetOSDOptions, _trt__GetOSDOptionsResponse &trt__GetOSDOptionsResponse)
{
    SOAP_EMPTY_HANDLER(trt__GetOSDOptions, "Media");
}



int MediaBindingService::SetOSD(_trt__SetOSD *trt__SetOSD, _trt__SetOSDResponse &trt__SetOSDResponse)
{
    SOAP_EMPTY_HANDLER(trt__SetOSD, "Media");
}



int MediaBindingService::CreateOSD(_trt__CreateOSD *trt__CreateOSD, _trt__CreateOSDResponse &trt__CreateOSDResponse)
{
    SOAP_EMPTY_HANDLER(trt__CreateOSD, "Media");
}



int MediaBindingService::DeleteOSD(_trt__DeleteOSD *trt__DeleteOSD, _trt__DeleteOSDResponse &trt__DeleteOSDResponse)
{
    SOAP_EMPTY_HANDLER(trt__DeleteOSD, "Media");
}
