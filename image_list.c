#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdio.h>
#include <string.h>

#define MAX_LINES 100
#define MAX_LEN 100
#define HISTORY_FILE "/home/anton/ClipboardHistory/history.txt"
#define HISTORY_DIR "/home/anton/ClipboardHistory"
#define IMG_DIR "/home/anton/ClipboardHistory/images"
#define TEXT_DIR "/home/anton/ClipboardHistory/texts"

static int read_history(char splited_lines[MAX_LINES][2][MAX_LEN])
{
    FILE *fptr = fopen(HISTORY_FILE, "r");
    if (!fptr)
    {
        printf("Not able to open the file.\n");
        return 0;
    }

    char line[MAX_LEN];
    int count = 0;

    while (count < MAX_LINES && fgets(line, MAX_LEN, fptr))
    {
        // remove newline
        line[strcspn(line, "\n")] = '\0';

        // remove leading spaces
        while (line[0] == ' ')
            memmove(line, line + 1, strlen(line));

        // split on first space
        char *token = strtok(line, " ");
        if (token)
        {
            strncpy(splited_lines[count][0], token, MAX_LEN);
            token = strtok(NULL, ""); // rest of line
            if (token)
            {
                strncpy(splited_lines[count][1], token, MAX_LEN);
            }
            else
            {
                splited_lines[count][1][0] = '\0';
            }
            count++;
        }
    }

    fclose(fptr);
    return count;
}

static gboolean quit_cb(gpointer user_data)
{
    g_application_quit(G_APPLICATION(user_data));
    return G_SOURCE_REMOVE;
}

static void load_css(const char *css_file)
{
    GtkCssProvider *provider = gtk_css_provider_new();

    GFile *file = g_file_new_for_path(css_file);
    gtk_css_provider_load_from_file(provider, file);
    g_object_unref(file);

    GdkDisplay *display = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(display,
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(provider);
}

static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    g_print("Row activated!\n");

    GtkWidget *frame = gtk_list_box_row_get_child(row);
    if (!frame)
        return;

    GtkWidget *hbox = gtk_frame_get_child(GTK_FRAME(frame));
    if (!hbox)
        return;

    GtkWidget *child = gtk_widget_get_first_child(hbox);

    GdkDisplay *display = gdk_display_get_default();
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);

    if (GTK_IS_LABEL(child))
    {
        // TEXT row
        const char *text = gtk_label_get_text(GTK_LABEL(child));
        if (text)
        {
            gdk_clipboard_set_text(clipboard, text);
            g_print("Copied text: %s\n", text);
        }
    }
    else if (GTK_IS_IMAGE(child))
    {
        // IMAGE row → load file path and copy as image
        const char *file_path = g_object_get_data(G_OBJECT(row), "file-path");
        if (file_path)
        {
            GError *err = NULL;
            GdkTexture *texture = gdk_texture_new_from_filename(file_path, &err);
            if (texture)
            {
                gdk_clipboard_set_texture(clipboard, texture);
                g_object_unref(texture);
                g_print("Copied image: %s\n", file_path);
            }
            else
            {
                g_warning("Failed to load image for clipboard: %s", err->message);
                g_clear_error(&err);
            }
        }
    }

    // quit after short delay
    g_timeout_add_once(100, (GSourceOnceFunc)g_application_quit, user_data);
}

// Create a single card row (image or text)
static GtkWidget *create_card(const char type[MAX_LEN], const char *content)
{
    GtkWidget *row = gtk_list_box_row_new();

    // Outer frame for card style
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "card");

    // Container inside frame
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(hbox, 10);
    gtk_widget_set_margin_end(hbox, 10);
    gtk_widget_set_margin_top(hbox, 10);
    gtk_widget_set_margin_bottom(hbox, 10);

    if (strcmp(type, "IMAGE") == 0)
    {
        GError *err = NULL;
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(content, &err);
        GtkWidget *image;
        if (pixbuf)
        {
            GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, 64, 64, GDK_INTERP_BILINEAR);
            image = gtk_image_new_from_pixbuf(scaled);
            gtk_widget_set_size_request(image, 64, 64);

            // Save pixbuf as PNG bytes for clipboard use
            gsize len = 0;
            guchar *png_data = NULL;
            if (gdk_pixbuf_save_to_buffer(pixbuf, (char **)&png_data, &len, "png", NULL, &err))
            {
                GBytes *bytes = g_bytes_new_take(png_data, len);
                g_object_set_data_full(G_OBJECT(row), "image-bytes", bytes, (GDestroyNotify)g_bytes_unref);
            }

            g_object_unref(scaled);
            g_object_unref(pixbuf);
        }
        else
        {
            image = gtk_image_new();
            g_clear_error(&err);
        }
        gtk_box_append(GTK_BOX(hbox), image);

        // Keep path too (optional)
        g_object_set_data_full(G_OBJECT(row), "file-path", g_strdup(content), g_free);
    }
    else if (strcmp(type, "TEXT") == 0)
    {
        FILE *tf = fopen(content, "r");
        char buffer[MAX_LEN] = "";
        if (tf)
        {
            if (fgets(buffer, sizeof(buffer), tf))
                buffer[strcspn(buffer, "\n")] = '\0';
            fclose(tf);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "Cannot open %s", content);
        }

        GtkWidget *label = gtk_label_new(buffer);
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_width_chars(GTK_LABEL(label), 25);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_append(GTK_BOX(hbox), label);
    }

    gtk_frame_set_child(GTK_FRAME(frame), hbox);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), frame);
    return row;
}

// Create the listbox and populate it
static GtkWidget *create_list(char splited_lines[MAX_LINES][2][MAX_LEN], int count)
{
    GtkWidget *listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);

    for (int i = 0; i < count; i++)
    {
        if (splited_lines[i][0][0] == '\0')
            continue;

        GtkWidget *row = create_card(splited_lines[i][0], splited_lines[i][1]);
        gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    }

    return listbox;
}

static GtkWidget *create_main_layout(GtkApplication *app,
                                     char splited_lines[MAX_LINES][2][MAX_LEN],
                                     int count)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Clipboard History");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);
    gtk_widget_set_margin_top(main_box, 10);
    gtk_widget_set_margin_bottom(main_box, 10);
    gtk_widget_add_css_class(main_box, "body");

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *listbox = create_list(splited_lines, count);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listbox);
    gtk_box_append(GTK_BOX(main_box), scrolled);

    gtk_window_set_child(GTK_WINDOW(window), main_box);

    g_signal_connect(listbox, "row-activated", G_CALLBACK(on_row_activated), app);

    return window;
}

// Activate handler
static void activate(GtkApplication *app, gpointer user_data)
{

    char splited_lines[MAX_LINES][2][MAX_LEN];
    int count = read_history(splited_lines);

    GtkWidget *window = create_main_layout(app, splited_lines, count);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "org.example.image_list", G_APPLICATION_DEFAULT_FLAGS);

    // Use CSS file from argument if given, else default to "style.css"
    const char *css_file = (argc > 1) ? argv[1] : "style.css";
    load_css(css_file);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}