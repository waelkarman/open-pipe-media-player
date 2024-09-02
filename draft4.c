#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gdk/gdk.h>

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
  GstElement *videosink;
  GstElement *gtkglsink;
  gboolean playing;      
  gboolean terminate;    
  gboolean seek_enabled; 
  gboolean seek_done;    
  gint64 duration;
  GtkWidget *sink_widget;       
} CustomData;

static void handle_message (CustomData *data, GstMessage *msg);
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

static void play_cb (GtkButton *button, CustomData *data) {
  gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

static void pause_cb (GtkButton *button, CustomData *data) {
  gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
}

static void stop_cb (GtkButton *button, CustomData *data) {
  gst_element_set_state (data->pipeline, GST_STATE_READY);
}

void create_ui(CustomData *data){
    GtkWidget *window;
    GtkWidget *main_view;
    GtkWidget *buttons;
    GtkWidget *play_button, *pause_button, *stop_button;

    buttons = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    play_button = gtk_button_new_with_label("Play");
    pause_button = gtk_button_new_with_label("Pause");
    stop_button = gtk_button_new_with_label("Stop");
    gtk_box_pack_start(GTK_BOX(buttons), play_button, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (play_button), "clicked", G_CALLBACK (play_cb), data);

    gtk_box_pack_start(GTK_BOX(buttons), pause_button, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (pause_button), "clicked", G_CALLBACK (pause_cb), data);

    gtk_box_pack_start(GTK_BOX(buttons), stop_button, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (stop_button), "clicked", G_CALLBACK (stop_cb), data);


    main_view = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX (main_view), data->sink_widget, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (main_view), buttons, TRUE, TRUE, 0);


    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Open pipe media player");
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_container_add(GTK_CONTAINER(window), main_view);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_widget_show_all(window);
};

void* gtk_main_loop(void* data) {
  CustomData* cstd = (CustomData*)data;
  gtk_main();
  cstd->terminate = TRUE;
}

int main(int argc, char *argv[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  data.terminate = FALSE;

  gtk_init(&argc, &argv);
  gst_init (&argc, &argv);

  data.seek_done = TRUE;
  data.source = gst_element_factory_make ("uridecodebin", "source");
  data.aconvert = gst_element_factory_make ("audioconvert", "audio-convert");
  data.resample = gst_element_factory_make ("audioresample", "resample");
  data.audio_queue = gst_element_factory_make("queue", "audio_queue");
  data.video_queue = gst_element_factory_make("queue", "video_queue");
  data.asink = gst_element_factory_make ("autoaudiosink", "audio-sink");
  data.vconvert = gst_element_factory_make ("videoconvert", "video-convert");
  data.videosink = gst_element_factory_make ("glsinkbin", "glsinkbin");
  data.gtkglsink = gst_element_factory_make ("gtkglsink", "gtkglsink");
  
  data.pipeline = gst_pipeline_new ("open-audio-video-pipeline");
  
  if (!data.pipeline || !data.videosink || !data.source || !data.aconvert || !data.audio_queue || !data.video_queue || !data.vconvert || !data.resample || !data.asink || !data.gtkglsink){
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }
  
  /* Select the source */
  if(argc > 1){
    size_t length = strlen(argv[1]);
    char *copy = malloc(7+length + 1);
    strcpy(copy, "file://");
    strcat(copy, argv[1]);
    g_object_set (data.source, "uri", copy, NULL);
  }else{
    g_object_set (data.source, "uri", "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", NULL);
  }


  /* Set gtk widget as sink video */
  #ifdef HWACC_ENABLED
    if (data.gtkglsink != NULL && data.videosink != NULL) {
      g_printerr ("Successfully created GTK GL Sink \n");
      g_object_set (data.videosink, "sink", data.gtkglsink, NULL);
      g_object_get (data.gtkglsink, "widget", &data.sink_widget, NULL);
    }else{
      return -1;
    }
  #else
      g_printerr ("Could not create gtkglsink, falling back to gtksink.\n");
      data.videosink = gst_element_factory_make ("gtksink", "gtksink");
      g_object_get (data.videosink, "widget", &data.sink_widget, NULL);
  #endif 


  /* Add gst-elements to BIN */
  gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.audio_queue, data.video_queue, data.aconvert, data.resample, data.asink, data.vconvert, data.videosink, NULL);
  
  /* Link separate audio pipeline branch */
  if (!gst_element_link_many (data.audio_queue, data.aconvert, data.resample, data.asink, NULL)) {
    g_printerr ("Elements could not be linked on audio brach.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Link separate video pipeline branch */
  if (!gst_element_link_many (data.video_queue, data.vconvert, data.videosink, NULL)) {
    g_printerr ("Elements could not be linked on video brach.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Connect source pads to pipeline on the fly depending on the content of the source */
  g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

  /* Create GUI */
  create_ui (&data);

  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }
  
  /* Run gtk main loop in a separate thread */
  pthread_t ui_loop_thread;
  pthread_create(&ui_loop_thread, NULL, &gtk_main_loop, (void*)&data);


  /* Pipeline query handler */
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

        g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r", GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));
        
        
        // if (data.seek_enabled && !data.seek_done && current > 5 * GST_SECOND) {
        //   g_print ("\nReached 10s, performing seek...\n");
        //   gst_element_seek_simple (data.pipeline, GST_FORMAT_TIME,GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 45 * GST_SECOND);
        //   data.seek_done = TRUE;
        // }

      }     
    }
  } while (!data.terminate);

  pthread_join(ui_loop_thread, NULL);

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
        if (new_pad_caps != NULL)
          gst_caps_unref (new_pad_caps);
        gst_object_unref (vsink_pad);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
  }

  GstPad *asink_pad = gst_element_get_static_pad (data->audio_queue, "sink");
  if(g_str_has_prefix (new_pad_type, "audio/x-raw")){
    ret = gst_pad_link (new_pad, asink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
      if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);
      gst_object_unref (asink_pad);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
  }
  
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
