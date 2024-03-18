#include <gst/gst.h>


typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *aconvert;
  GstElement *vconvert;
  GstElement *resample;
  GstElement *audio_queue;
  GstElement *video_queue;
  GstElement *asink;
  GstElement *vsink;
  gboolean playing;      
  gboolean terminate;    
  gboolean seek_enabled; 
  gboolean seek_done;    
  gint64 duration;       
} CustomData;

static void handle_message (CustomData *data, GstMessage *msg);
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  gst_init (&argc, &argv);

  
  data.source = gst_element_factory_make ("uridecodebin", "source");
  data.aconvert = gst_element_factory_make ("audioconvert", "audio-convert");
  data.resample = gst_element_factory_make ("audioresample", "resample");
  data.audio_queue = gst_element_factory_make("queue", "audio_queue");
  data.video_queue = gst_element_factory_make("queue", "video_queue");
  data.asink = gst_element_factory_make ("autoaudiosink", "audio-sink");
  data.vconvert = gst_element_factory_make ("videoconvert", "vido-convert");
  data.vsink = gst_element_factory_make ("autovideosink", "video-sink");

  
  data.pipeline = gst_pipeline_new ("test-pipeline");

  if (!data.pipeline || !data.source || !data.aconvert || !data.audio_queue || !data.video_queue || !data.vconvert || !data.resample || !data.asink || !data.vsink){
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.audio_queue, data.video_queue, data.aconvert, data.resample, data.asink, data.vconvert, data.vsink, NULL);
  
  if (!gst_element_link_many (data.audio_queue, data.aconvert, data.resample, data.asink, NULL)) {
    g_printerr ("Elements could not be linked on audio brach.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  if (!gst_element_link_many (data.video_queue, data.vconvert, data.vsink, NULL)) {
    g_printerr ("Elements could not be linked on video brach.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  g_object_set (data.source, "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

  g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

  
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, 100*GST_MSECOND,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);

    if (msg != NULL) {
      handle_message (&data, msg);
    }
    else
    {
      
      if (data.playing) {
        gint64 current = -1;

        
        if (!gst_element_query_position (data.pipeline, GST_FORMAT_TIME, &current)) {
          g_printerr ("Could not query current position.\n");
        }

        
        if (!GST_CLOCK_TIME_IS_VALID (data.duration)) {
          if (!gst_element_query_duration (data.pipeline, GST_FORMAT_TIME, &data.duration)) {
            g_printerr ("Could not query current duration.\n");
          }
        }

        
        g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
            GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));

        
        if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
          g_print ("\nReached 10s, performing seek...\n");
          gst_element_seek_simple (data.pipeline, GST_FORMAT_TIME,
              GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
          data.seek_done = TRUE;
        }
      }
    }
  } while (!terminate);

  
  gst_object_unref (bus);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}


static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);

  GstPad *vsink_pad = gst_element_get_static_pad (data->video_queue, "sink");
  if(g_str_has_prefix (new_pad_type, "video/x-raw")){
    ret = gst_pad_link (new_pad, vsink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
  }

  GstPad *asink_pad = gst_element_get_static_pad (data->audio_queue, "sink");
  if(g_str_has_prefix (new_pad_type, "audio/x-raw")){
    ret = gst_pad_link (new_pad, asink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
  }

exit:
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  
  gst_object_unref (asink_pad);
  gst_object_unref (vsink_pad);
}


static void handle_message (CustomData *data, GstMessage *msg) {
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_EOS:
      g_print ("\nEnd-Of-Stream reached.\n");
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_DURATION:
      
      data->duration = GST_CLOCK_TIME_NONE;
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

        
        data->playing = (new_state == GST_STATE_PLAYING);

        if (data->playing) {
          
          GstQuery *query;
          gint64 start, end;
          query = gst_query_new_seeking (GST_FORMAT_TIME);
          if (gst_element_query (data->pipeline, query)) {
            gst_query_parse_seeking (query, NULL, &data->seek_enabled, &start, &end);
            if (data->seek_enabled) {
              g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
                  GST_TIME_ARGS (start), GST_TIME_ARGS (end));
            } else {
              g_print ("Seeking is DISABLED for this stream.\n");
            }
          }
          else {
            g_printerr ("Seeking query failed.");
          }
          gst_query_unref (query);
        }
      }
    } break;
    default:
      
      g_printerr ("Unexpected message received.\n");
      break;
  }
  gst_message_unref (msg);
}