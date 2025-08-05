#include <switch.h>
#include <switch_utils.h>
#include "signalsmith-stretch/plugin/stretch-vocal.h"  // Updated include path

// Forward declarations
SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown);
SWITCH_MODULE_DEFINITION(mod_voicechanger, mod_voicechanger_load, mod_voicechanger_shutdown, NULL);
SWITCH_STANDARD_API(voicechanger_api);

struct voicechanger_context {
    StretchVocalUp *stretch;  // Removed namespace since class is in global scope
    int sample_rate;
    bool enabled;
    bool is_aparty;

    voicechanger_context() : 
        stretch(nullptr),
        sample_rate(0),
        enabled(true),
        is_aparty(false) {}
};

static switch_bool_t voicechanger_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type) {
    auto *context = (voicechanger_context *)user_data;
    switch_core_session_t *session = switch_core_media_bug_get_session(bug);
    switch_channel_t *channel = switch_core_session_get_channel(session);

    switch (type) {
        case SWITCH_ABC_TYPE_INIT: {
            const char *direction = switch_channel_get_variable(channel, "direction");
            context->is_aparty = (direction && !strcmp(direction, "outbound"));
            
            switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
            context->sample_rate = read_codec->implementation->actual_samples_per_second;
            int channels = read_codec->implementation->number_of_channels;

            if (channels != 1) {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                    "VoiceChanger only supports mono audio (got %d channels)\n", channels);
                context->enabled = false;
                return SWITCH_TRUE;
            }

            context->stretch = new StretchVocalUp();  // Removed namespace
            context->stretch->configure(context->sample_rate, 2048);

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
                "VoiceChanger INIT: %d Hz, %s\n",
                context->sample_rate, 
                context->is_aparty ? "A-party" : "B-party");

            break;
        }

        case SWITCH_ABC_TYPE_CLOSE: {
            if (context->stretch) {
                delete context->stretch;
                context->stretch = nullptr;
            }
            break;
        }

        case SWITCH_ABC_TYPE_WRITE_REPLACE: {
            if (!context->is_aparty || !context->enabled || !context->stretch) {
                return SWITCH_TRUE;
            }

            switch_frame_t *frame = switch_core_media_bug_get_write_replace_frame(bug);
            if (!frame || !frame->data) break;

            context->stretch->process((int16_t *)frame->data, frame->samples);
            break;
        }

        default: break;
    }

    return SWITCH_TRUE;
}

SWITCH_STANDARD_API(voicechanger_api) {
    switch_channel_t *channel = nullptr;
    switch_media_bug_t *bug = nullptr;
    switch_status_t status;
    char *mycmd = nullptr, *argv[10] = { 0 };
    int argc = 0;

    if (!zstr(cmd) && (mycmd = strdup(cmd))) {
        argc = switch_separate_string(mycmd, ' ', argv, 10);
    }

    if (argc < 2) {
        stream->write_function(stream, "-USAGE: voicechanger start|stop|status <uuid>\n");
        goto done;
    }

    if (!(session = switch_core_session_locate(argv[1]))) {
        stream->write_function(stream, "-ERR No such session!\n");
        goto done;
    }

    channel = switch_core_session_get_channel(session);

    if (!strcasecmp(argv[0], "status")) {
        if (switch_channel_get_private(channel, "voicechanger_bug")) {
            stream->write_function(stream, "+OK Running\n");
        } else {
            stream->write_function(stream, "-ERR Not running\n");
        }
        goto done;
    }

    if (!strcasecmp(argv[0], "stop")) {
        if ((bug = (switch_media_bug_t *)switch_channel_get_private(channel, "voicechanger_bug"))) {
            switch_channel_set_private(channel, "voicechanger_bug", nullptr);
            switch_core_media_bug_remove(session, &bug);
            stream->write_function(stream, "+OK Stopped\n");
        } else {
            stream->write_function(stream, "-ERR Not running\n");
        }
        goto done;
    }

    if (!strcasecmp(argv[0], "start")) {
        if (switch_channel_get_private(channel, "voicechanger_bug")) {
            stream->write_function(stream, "-ERR Already running\n");
            goto done;
        }

        voicechanger_context *context = (voicechanger_context *)switch_core_session_alloc(session, sizeof(voicechanger_context));
        new (context) voicechanger_context();

        status = switch_core_media_bug_add(session, "voicechanger", nullptr,
            voicechanger_callback, context, 0,
            SMBF_WRITE_REPLACE,
            &bug);

        if (status != SWITCH_STATUS_SUCCESS) {
            stream->write_function(stream, "-ERR Could not add media bug\n");
            goto done;
        }

        switch_channel_set_private(channel, "voicechanger_bug", bug);
        stream->write_function(stream, "+OK Started\n");
    } else {
        stream->write_function(stream, "-USAGE: voicechanger start|stop|status <uuid>\n");
    }

done:
    if (session) switch_core_session_rwunlock(session);
    switch_safe_free(mycmd);
    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load) {
    switch_api_interface_t *api_interface;

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    SWITCH_ADD_API(api_interface, "voicechanger", "Voice Changer API",
        voicechanger_api, "start|stop|status <uuid>");

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, 
        "VoiceChanger module loaded (A-party only mode) with fixed vocal effect\n");

    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VoiceChanger module unloaded\n");
    return SWITCH_STATUS_SUCCESS;
}
