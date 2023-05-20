/* search.c
 *
 * COMPILE
 *
 * gcc `pkg-config --cflags gtk4` -o search search.c `pkg-config --libs gtk4`
 *
 * RUN
 *
 * ./search
 *
 */

#include <gtk/gtk.h>

/* Used with foreach. Add a string to a list if the string contains a given
 * substring.
 *
 * data - The string we want to search through (the "haystack").
 *
 * user_data - A pointer to a list pointer (so that the old pointer can be
 * updated if needed). The list initially contains one item, the string
 * we want to find (the "needle"). A string containing the needle is 
 * appended to the list each time this function is called by foreach.
 *
 */
static void append_if_match_cb( gpointer data, gpointer user_data )
{
	gchar *haystack = (gchar *) data;
	GList **list_ptr = (GList **) user_data;

	g_assert( list_ptr );

	/* The first string in the list is the string we want to search for,
	 * i.e. the "needle".
	 */
	GList *first = g_list_first( *list_ptr );
	gchar *needle = (gchar *) first->data;

	/* Matches are appended to the end of the list.
	 */
	gssize haystack_len = -1; // search the whole string, like strstr

	if ( g_strstr_len( haystack, haystack_len, needle ) )
		*list_ptr = g_list_append( *list_ptr, haystack );
}

/* Find a matching string ("needle") in a list of strings ("haystacks").
 *
 * haystacks - list of strings to search for needle
 * needle - string to find in haystacks
 */
GList *find_strings_with_substring( GList *haystacks, gchar *needle )
{
	/* Create a list with the needle as the only element, like
	 * append_if_match expects.
	 */
	GList *found = NULL;

	/* Append the needle to the empty list, like append_if_match_cb expects.
	 */
	found = g_list_append( found, needle );

	/* Find any matching strings in the list of strings, and append them to
	 * the list of found strings.
	 */
	g_list_foreach( haystacks, append_if_match_cb, &found );

	/* Remove the first element, which is the search query.
	 */
	found = g_list_remove( found, (gconstpointer) needle );

	return found;
}


static GList *get_strings()
{
	GList *strings = NULL;

	strings = g_list_append( strings, "a" );
	strings = g_list_append( strings, "ab" );
	strings = g_list_append( strings, "abc" );
	strings = g_list_append( strings, "abcd" );

	return strings;
}

static void search_append_label( gpointer data, gpointer user_data )
{
	gchar *text;
	GtkWidget *listbox;

	text = (gchar *) data;
	listbox = GTK_WIDGET( user_data );

	gtk_box_append( GTK_BOX( listbox ), gtk_label_new( text ) );
}

static void search_changed( GtkWidget *search, gpointer user_data )
{
	GtkWidget *box, *listbox, *label;
	GList *strings, *found;
	const gchar *text;

	box = GTK_WIDGET( user_data );

	listbox = gtk_widget_get_last_child( box );

	gtk_widget_unparent( listbox );

	listbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_box_append( GTK_BOX( box ), listbox );

	strings = get_strings();

	text = gtk_editable_get_text( GTK_EDITABLE( search ) );

	found = find_strings_with_substring( strings, (gchar *) text );

	g_list_foreach( found, search_append_label, listbox );
}

static void
activate( GtkApplication *app, gpointer user_data )
{
	GtkWidget *window, *box, *search, *initial_listbox;
	GList *strings;

	char* placeholder_text = "hello world";

	window = gtk_application_window_new( app );

	box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_window_set_child( GTK_WINDOW( window ), box );

	search = gtk_search_new();

	g_signal_connect( search, "search-changed",
		G_CALLBACK( search_changed ), box );

	gtk_box_append( GTK_BOX( box ), search );

	initial_listbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	strings = get_strings();

	g_list_foreach( strings, search_append_label, initial_listbox );

	gtk_box_append( GTK_BOX( box ), initial_listbox );

	gtk_widget_show( window );
}

int
main( int argc, char** argv )
{
	GtkApplication *app; 
	int status;
	
	app = gtk_application_new( "org.gtk.example", G_APPLICATION_FLAGS_NONE );
	g_signal_connect( app, "activate", G_CALLBACK( activate ), NULL );
	status = g_application_run( G_APPLICATION( app ), argc, argv );
	g_object_unref( app );

	return status;
}
