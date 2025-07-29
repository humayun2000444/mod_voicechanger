// #include <switch.h>
// #include <vector>
// #include <switch_utils.h>
// #include "signalsmith-stretch/signalsmith-stretch.h"

// using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

// SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load);
// SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown);
// SWITCH_MODULE_DEFINITION(mod_voicechanger, mod_voicechanger_load, mod_voicechanger_shutdown, NULL);

// struct voicechanger_context {
//     Stretch *stretch;
//     float pitch_shift;
//     int sample_rate;
//     int channels;
//     bool enabled;
//     int buf_size;

//     std::vector<float*> in_ptrs;
//     std::vector<float*> out_ptrs;
// };

// static switch_bool_t voicechanger_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type) {
//     auto *context = (voicechanger_context *)user_data;
//     switch_core_session_t *session = switch_core_media_bug_get_session(bug);

//     switch (type) {
//         case SWITCH_ABC_TYPE_INIT: {
//             switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
//             context->sample_rate = read_codec->implementation->actual_samples_per_second;
//             context->channels = read_codec->implementation->number_of_channels;

//             context->stretch = new Stretch();
//             context->stretch->presetDefault(context->channels, context->sample_rate);
//             context->stretch->setTransposeFactor(context->pitch_shift);

//             context->buf_size = 2048;

//             context->in_ptrs.resize(context->channels);
//             context->out_ptrs.resize(context->channels);

//             for (int c = 0; c < context->channels; ++c) {
//                 context->in_ptrs[c] = (float *)switch_core_session_alloc(session, sizeof(float) * context->buf_size);
//                 context->out_ptrs[c] = (float *)switch_core_session_alloc(session, sizeof(float) * context->buf_size);
//             }

//             switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
//                 "VoiceChanger INIT: %d Hz, %d channels\n",
//                 context->sample_rate, context->channels);

//             break;
//         }

//         case SWITCH_ABC_TYPE_CLOSE: {
//             if (context->stretch) {
//                 delete context->stretch;
//                 context->stretch = nullptr;
//             }
//             break;
//         }

//         case SWITCH_ABC_TYPE_READ_REPLACE:
//         case SWITCH_ABC_TYPE_WRITE_REPLACE: {
//             switch_frame_t *frame = (type == SWITCH_ABC_TYPE_READ_REPLACE)
//                 ? switch_core_media_bug_get_read_replace_frame(bug)
//                 : switch_core_media_bug_get_write_replace_frame(bug);

//             if (!frame || !frame->data || !context->stretch || !context->enabled) break;

//             int samples = frame->samples;
//             int channels = context->channels;

//             if (samples > context->buf_size) {
//                 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
//                     "VoiceChanger: frame samples (%d) exceed buffer size (%d)\n",
//                     samples, context->buf_size);
//                 break;
//             }

//             int16_t *pcm_in = (int16_t *)frame->data;

//             for (int c = 0; c < channels; ++c) {
//                 for (int i = 0; i < samples; ++i) {
//                     context->in_ptrs[c][i] = pcm_in[i * channels + c] / 32768.0f;
//                 }
//             }

//             context->stretch->process(context->in_ptrs.data(), samples,
//                                       context->out_ptrs.data(), samples);

//             for (int c = 0; c < channels; ++c) {
//                 for (int i = 0; i < samples; ++i) {
//                     float sample = context->out_ptrs[c][i];
//                     if (sample > 1.0f) sample = 1.0f;
//                     if (sample < -1.0f) sample = -1.0f;
//                     pcm_in[i * channels + c] = (int16_t)(sample * 32767.0f);
//                 }
//             }

//             break;
//         }

//         default: break;
//     }

//     return SWITCH_TRUE;
// }

// SWITCH_STANDARD_API(voicechanger_api) {
//     switch_channel_t *channel = nullptr;
//     switch_media_bug_t *bug = nullptr;
//     switch_status_t status;
//     char *mycmd = nullptr, *argv[10] = { 0 };
//     int argc = 0;

//     if (!zstr(cmd) && (mycmd = strdup(cmd))) {
//         argc = switch_separate_string(mycmd, ' ', argv, 10);
//     }

//     if (argc < 2) {
//         stream->write_function(stream, "-USAGE: voicechanger start|stop|set <uuid> [pitch_shift]\n");
//         goto done;
//     }

//     if (!(session = switch_core_session_locate(argv[1]))) {
//         stream->write_function(stream, "-ERR No such session!\n");
//         goto done;
//     }

//     channel = switch_core_session_get_channel(session);

//     if (!strcasecmp(argv[0], "stop")) {
//         if ((bug = (switch_media_bug_t *)switch_channel_get_private(channel, "voicechanger_bug"))) {
//             switch_channel_set_private(channel, "voicechanger_bug", nullptr);
//             switch_core_media_bug_remove(session, &bug);
//             stream->write_function(stream, "+OK Stopped\n");
//         } else {
//             stream->write_function(stream, "-ERR Not running\n");
//         }
//         goto done;
//     }

//     if (!strcasecmp(argv[0], "start")) {
//         if (switch_channel_get_private(channel, "voicechanger_bug")) {
//             stream->write_function(stream, "-ERR Already running\n");
//             goto done;
//         }

//         voicechanger_context *context = (voicechanger_context *)switch_core_session_alloc(session, sizeof(voicechanger_context));
//         memset(context, 0, sizeof(voicechanger_context));
//         context->pitch_shift = (argc > 2) ? atof(argv[2]) : 1.0f;
//         context->enabled = true;

//         status = switch_core_media_bug_add(session, "voicechanger", nullptr,
//             voicechanger_callback, context, 0,
//             SMBF_READ_REPLACE | SMBF_WRITE_REPLACE,
//             &bug);


//         if (status != SWITCH_STATUS_SUCCESS) {
//             stream->write_function(stream, "-ERR Could not add media bug\n");
//             goto done;
//         }

//         switch_channel_set_private(channel, "voicechanger_bug", bug);
//         stream->write_function(stream, "+OK Started\n");

//     } else if (!strcasecmp(argv[0], "set")) {
//         if (!(bug = (switch_media_bug_t *)switch_channel_get_private(channel, "voicechanger_bug"))) {
//             stream->write_function(stream, "-ERR Not running\n");
//             goto done;
//         }

//         auto *context = (voicechanger_context *)switch_core_media_bug_get_user_data(bug);

//         if (argc > 2) context->pitch_shift = atof(argv[2]);

//         if (context->stretch) {
//             context->stretch->setTransposeFactor(context->pitch_shift);
//         }

//         stream->write_function(stream, "+OK Updated\n");

//     } else {
//         stream->write_function(stream, "-USAGE: voicechanger start|stop|set <uuid> [pitch_shift]\n");
//     }

// done:
//     if (session) switch_core_session_rwunlock(session);
//     switch_safe_free(mycmd);
//     return SWITCH_STATUS_SUCCESS;
// }

// SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load) {
//     switch_api_interface_t *api_interface;

//     *module_interface = switch_loadable_module_create_module_interface(pool, modname);

//     SWITCH_ADD_API(api_interface, "voicechanger", "Voice Changer API",
//         voicechanger_api, "start|stop|set <uuid> [pitch_shift]");

//     switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VoiceChanger module loaded\n");

//     return SWITCH_STATUS_SUCCESS;
// }

// SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown) {
//     switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VoiceChanger module unloaded\n");
//     return SWITCH_STATUS_SUCCESS;
// }

//al party



// #include <switch.h>
// #include <vector>
// #include <switch_utils.h>
// #include "signalsmith-stretch/signalsmith-stretch.h"

// using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

// SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load);
// SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown);
// SWITCH_MODULE_DEFINITION(mod_voicechanger, mod_voicechanger_load, mod_voicechanger_shutdown, NULL);

// struct voicechanger_context {
//     Stretch *stretch;
//     float pitch_shift;
//     int sample_rate;
//     int channels;
//     bool enabled;
//     int buf_size;

//     std::vector<float*> in_ptrs;
//     std::vector<float*> out_ptrs;

//     // Constructor for proper initialization
//     voicechanger_context() : 
//         stretch(nullptr),
//         pitch_shift(1.0f),
//         sample_rate(0),
//         channels(0),
//         enabled(true),
//         buf_size(0) {}
// };

// static switch_bool_t voicechanger_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type) {
//     auto *context = (voicechanger_context *)user_data;
//     switch_core_session_t *session = switch_core_media_bug_get_session(bug);

//     switch (type) {
//         case SWITCH_ABC_TYPE_INIT: {
//             switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
//             context->sample_rate = read_codec->implementation->actual_samples_per_second;
//             context->channels = read_codec->implementation->number_of_channels;

//             context->stretch = new Stretch();
//             context->stretch->presetDefault(context->channels, context->sample_rate);
//             context->stretch->setTransposeFactor(context->pitch_shift);

//             context->buf_size = 2048;

//             context->in_ptrs.resize(context->channels);
//             context->out_ptrs.resize(context->channels);

//             for (int c = 0; c < context->channels; ++c) {
//                 context->in_ptrs[c] = (float *)switch_core_session_alloc(session, sizeof(float) * context->buf_size);
//                 context->out_ptrs[c] = (float *)switch_core_session_alloc(session, sizeof(float) * context->buf_size);
//             }

//             switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
//                 "VoiceChanger INIT: %d Hz, %d channels\n",
//                 context->sample_rate, context->channels);

//             break;
//         }

//         case SWITCH_ABC_TYPE_CLOSE: {
//             if (context->stretch) {
//                 delete context->stretch;
//                 context->stretch = nullptr;
//             }
//             break;
//         }

//         case SWITCH_ABC_TYPE_WRITE_REPLACE: {
//             switch_frame_t *frame = switch_core_media_bug_get_write_replace_frame(bug);

//             if (!frame || !frame->data || !context->stretch || !context->enabled) break;

//             int samples = frame->samples;
//             int channels = context->channels;

//             if (samples > context->buf_size) {
//                 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
//                     "VoiceChanger: frame samples (%d) exceed buffer size (%d)\n",
//                     samples, context->buf_size);
//                 break;
//             }

//             int16_t *pcm_in = (int16_t *)frame->data;

//             for (int c = 0; c < channels; ++c) {
//                 for (int i = 0; i < samples; ++i) {
//                     context->in_ptrs[c][i] = pcm_in[i * channels + c] / 32768.0f;
//                 }
//             }

//             context->stretch->process(context->in_ptrs.data(), samples,
//                                     context->out_ptrs.data(), samples);

//             for (int c = 0; c < channels; ++c) {
//                 for (int i = 0; i < samples; ++i) {
//                     float sample = context->out_ptrs[c][i];
//                     if (sample > 1.0f) sample = 1.0f;
//                     if (sample < -1.0f) sample = -1.0f;
//                     pcm_in[i * channels + c] = (int16_t)(sample * 32767.0f);
//                 }
//             }

//             break;
//         }

//         default: break;
//     }

//     return SWITCH_TRUE;
// }

// SWITCH_STANDARD_API(voicechanger_api) {
//     switch_channel_t *channel = nullptr;
//     switch_media_bug_t *bug = nullptr;
//     switch_status_t status;
//     char *mycmd = nullptr, *argv[10] = { 0 };
//     int argc = 0;

//     if (!zstr(cmd) && (mycmd = strdup(cmd))) {
//         argc = switch_separate_string(mycmd, ' ', argv, 10);
//     }

//     if (argc < 2) {
//         stream->write_function(stream, "-USAGE: voicechanger start|stop|set <uuid> [pitch_shift]\n");
//         goto done;
//     }

//     if (!(session = switch_core_session_locate(argv[1]))) {
//         stream->write_function(stream, "-ERR No such session!\n");
//         goto done;
//     }

//     channel = switch_core_session_get_channel(session);

//     if (!strcasecmp(argv[0], "stop")) {
//         if ((bug = (switch_media_bug_t *)switch_channel_get_private(channel, "voicechanger_bug"))) {
//             switch_channel_set_private(channel, "voicechanger_bug", nullptr);
//             switch_core_media_bug_remove(session, &bug);
//             stream->write_function(stream, "+OK Stopped\n");
//         } else {
//             stream->write_function(stream, "-ERR Not running\n");
//         }
//         goto done;
//     }

//     if (!strcasecmp(argv[0], "start")) {
//         if (switch_channel_get_private(channel, "voicechanger_bug")) {
//             stream->write_function(stream, "-ERR Already running\n");
//             goto done;
//         }

//         voicechanger_context *context = (voicechanger_context *)switch_core_session_alloc(session, sizeof(voicechanger_context));
//         new (context) voicechanger_context(); // Proper initialization
//         context->pitch_shift = (argc > 2) ? atof(argv[2]) : 1.0f;

//         status = switch_core_media_bug_add(session, "voicechanger", nullptr,
//             voicechanger_callback, context, 0,
//             SMBF_WRITE_REPLACE,
//             &bug);

//         if (status != SWITCH_STATUS_SUCCESS) {
//             stream->write_function(stream, "-ERR Could not add media bug\n");
//             goto done;
//         }

//         switch_channel_set_private(channel, "voicechanger_bug", bug);
//         stream->write_function(stream, "+OK Started\n");

//     } else if (!strcasecmp(argv[0], "set")) {
//         if (!(bug = (switch_media_bug_t *)switch_channel_get_private(channel, "voicechanger_bug"))) {
//             stream->write_function(stream, "-ERR Not running\n");
//             goto done;
//         }

//         auto *context = (voicechanger_context *)switch_core_media_bug_get_user_data(bug);

//         if (argc > 2) context->pitch_shift = atof(argv[2]);

//         if (context->stretch) {
//             context->stretch->setTransposeFactor(context->pitch_shift);
//         }

//         stream->write_function(stream, "+OK Updated\n");

//     } else {
//         stream->write_function(stream, "-USAGE: voicechanger start|stop|set <uuid> [pitch_shift]\n");
//     }

// done:
//     if (session) switch_core_session_rwunlock(session);
//     switch_safe_free(mycmd);
//     return SWITCH_STATUS_SUCCESS;
// }

// SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load) {
//     switch_api_interface_t *api_interface;

//     *module_interface = switch_loadable_module_create_module_interface(pool, modname);

//     SWITCH_ADD_API(api_interface, "voicechanger", "Voice Changer API",
//         voicechanger_api, "start|stop|set <uuid> [pitch_shift]");

//     switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VoiceChanger module loaded\n");

//     return SWITCH_STATUS_SUCCESS;
// }

// SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown) {
//     switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VoiceChanger module unloaded\n");
//     return SWITCH_STATUS_SUCCESS;
// }

// ?? working 

#include <switch.h>
#include <vector>
#include <switch_utils.h>
#include "signalsmith-stretch/signalsmith-stretch.h"

using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

// Forward declarations
SWITCH_MODULE_LOAD_FUNCTION(mod_voicechanger_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown);
SWITCH_MODULE_DEFINITION(mod_voicechanger, mod_voicechanger_load, mod_voicechanger_shutdown, NULL);
SWITCH_STANDARD_API(voicechanger_api);

struct voicechanger_context {
    Stretch *stretch;
    float pitch_shift;
    int sample_rate;
    int channels;
    bool enabled;
    int buf_size;
    bool is_aparty;

    std::vector<float*> in_ptrs;
    std::vector<float*> out_ptrs;

    voicechanger_context() : 
        stretch(nullptr),
        pitch_shift(1.0f),
        sample_rate(0),
        channels(0),
        enabled(true),
        buf_size(0),
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
            context->channels = read_codec->implementation->number_of_channels;

            context->stretch = new Stretch();
            context->stretch->presetDefault(context->channels, context->sample_rate);
            context->stretch->setTransposeFactor(context->pitch_shift);

            context->buf_size = 2048;
            context->in_ptrs.resize(context->channels);
            context->out_ptrs.resize(context->channels);

            for (int c = 0; c < context->channels; ++c) {
                context->in_ptrs[c] = (float *)switch_core_session_alloc(session, sizeof(float) * context->buf_size);
                context->out_ptrs[c] = (float *)switch_core_session_alloc(session, sizeof(float) * context->buf_size);
            }

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
                "VoiceChanger INIT: %d Hz, %d channels, %s, pitch: %.1f\n",
                context->sample_rate, context->channels, 
                context->is_aparty ? "A-party" : "B-party",
                context->pitch_shift);

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
            if (!context->is_aparty || !context->enabled) {
                return SWITCH_TRUE;
            }

            switch_frame_t *frame = switch_core_media_bug_get_write_replace_frame(bug);

            if (!frame || !frame->data || !context->stretch) break;

            int samples = frame->samples;
            int channels = context->channels;

            if (samples > context->buf_size) {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                    "VoiceChanger: frame samples (%d) exceed buffer size (%d)\n",
                    samples, context->buf_size);
                break;
            }

            int16_t *pcm_in = (int16_t *)frame->data;

            for (int c = 0; c < channels; ++c) {
                for (int i = 0; i < samples; ++i) {
                    context->in_ptrs[c][i] = pcm_in[i * channels + c] / 32768.0f;
                }
            }

            context->stretch->process(context->in_ptrs.data(), samples,
                                    context->out_ptrs.data(), samples);

            for (int c = 0; c < channels; ++c) {
                for (int i = 0; i < samples; ++i) {
                    float sample = context->out_ptrs[c][i];
                    if (sample > 1.0f) sample = 1.0f;
                    if (sample < -1.0f) sample = -1.0f;
                    pcm_in[i * channels + c] = (int16_t)(sample * 32767.0f);
                }
            }

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
        stream->write_function(stream, "-USAGE: voicechanger start|stop|set <uuid> [pitch_shift]\n");
        goto done;
    }

    if (!(session = switch_core_session_locate(argv[1]))) {
        stream->write_function(stream, "-ERR No such session!\n");
        goto done;
    }

    channel = switch_core_session_get_channel(session);

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
        context->pitch_shift = (argc > 2) ? atof(argv[2]) : 1.0f;

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

    } else if (!strcasecmp(argv[0], "set")) {
        if (!(bug = (switch_media_bug_t *)switch_channel_get_private(channel, "voicechanger_bug"))) {
            stream->write_function(stream, "-ERR Not running\n");
            goto done;
        }

        auto *context = (voicechanger_context *)switch_core_media_bug_get_user_data(bug);

        if (argc > 2) context->pitch_shift = atof(argv[2]);

        if (context->stretch) {
            context->stretch->setTransposeFactor(context->pitch_shift);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
                "API set pitch to: %.1f\n", context->pitch_shift);
        }

        stream->write_function(stream, "+OK Updated\n");

    } else {
        stream->write_function(stream, "-USAGE: voicechanger start|stop|set <uuid> [pitch_shift]\n");
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
        voicechanger_api, "start|stop|set <uuid> [pitch_shift]");

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, 
        "VoiceChanger module loaded (A-party only mode)\n");

    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_voicechanger_shutdown) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VoiceChanger module unloaded\n");
    return SWITCH_STATUS_SUCCESS;
}