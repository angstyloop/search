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

/* This a the callback passed to foreach. Add a string to a list if the string
 * contains a given substring.
 *
 * data - The string we want to search through (the "haystack").
 *
 * user_data - A pointer to a list pointer (so that the old pointer can be
 * updated if needed). The list initially contains one item, the string
 * we want to find (the "needle"). A string containing the needle is 
 * appended to the list each time this function is called by foreach.
 *
 */
static void
append_if_match( gpointer data, gpointer user_data )
{
	GRegex *regex;
	GMatchInfo *match_info;
	gchar *escaped_needle;
	GList *first, *second;
	gchar *haystack, *needle;
	GList **list_ptr;
	gboolean use_regex;

	list_ptr = (GList **) user_data;

	haystack = (gchar *) data;

	/* The first element in the list is the string we want to search for,
	 * i.e. the "needle".
	 */
	first = g_list_first( *list_ptr );
	needle = (gchar *) first->data;

	/* The second element is a boolean that determines whether or not we
	 * interpret regex in the needle.
	 */
	second = first->next;
	use_regex = GPOINTER_TO_INT( second->data );

	/* GMatchInfo for strings containin the substring are appended to the
	 * end of the list. 
	 */
	gssize haystack_len = -1; // search the whole string, like strstr

	/* Optionally escape regex special characters
	 */
	escaped_needle = use_regex
		? g_strdup( needle )
		: g_regex_escape_string( needle, -1  );

	/* Create a regex to search for the matching substring.
	 */
	regex = g_regex_new( escaped_needle, 0, 0, NULL );

	/* If the regex compiled...
	 */
	if ( regex ) {
		/* If the string @haystack matches @regex, append the resulting
		 * @match_info to the list.
		 */
		if ( g_regex_match( regex, haystack, 0, &match_info ) )
			*list_ptr = g_list_append( *list_ptr, match_info );

		g_regex_unref( regex );

	}
}

/* Find a matching string ("needle") in a list of strings ("haystacks").
 *
 * haystacks - list of strings to search for needle
 * needle - string to find in haystacks
 */
GList *
find_strings_with_substring( GList *haystacks, gchar *needle, gboolean regex_enabled )
{
	/* Create a list with the needle as the only element, like
	 * append_if_match expects.
	 */
	GList *found = NULL;

	/* Append @needle and @regex_enabled to the list, like
	 * append_if_match_cb expects.
	 */
	found = g_list_append( found, needle );
	found = g_list_append( found, GINT_TO_POINTER( regex_enabled ) );

	/* Find any matching strings in the list of strings, and append them to
	 * the list of found strings.
	 */
	g_list_foreach( haystacks, append_if_match, &found );

	/* Remove the first element, which is the search query, a gchar*.
	 */
	found = g_list_remove( found, (gconstpointer) needle );

	/* Remove the second element, which is a gboolean that decides whether
	 * or not we interpret regex in the search query.
	 */
	found = g_list_remove( found, GINT_TO_POINTER( regex_enabled ) );

	return found;
}

/* This method shouldn't be reused in other code - it just generates a list of
 * string we can use to test the reusable code. 
 */
static GList *
get_strings()
{
	GList *strings = NULL;

	strings = g_list_append( strings, "a" );
	strings = g_list_append( strings, "ab" );
	strings = g_list_append( strings, "abc" );
	strings = g_list_append( strings, "abcd" );
	strings = g_list_append( strings, "aa" );
	strings = g_list_append( strings, "abab" );
	strings = g_list_append( strings, "abcabc" );
	strings = g_list_append( strings, "abcdabcd" );

	return strings;
}

/* Get a string with the matching substrings marked up.
 *
 * match_info - The object containing the pointers to the matching substrings
 * for a given searched string.
 */
static gchar *
get_markup_from_match_info( GMatchInfo *match_info  )
{
	const gchar *raw_str, *result, *match_str;
	gchar *str;
	gchar *left_token, *right_token;
	GRegex *regex;

	left_token = "<b>";

	right_token = "</b>";

	regex = g_match_info_get_regex( match_info );

	match_str = g_regex_get_pattern( regex );

	raw_str = g_match_info_get_string( match_info );

	gchar *replacement =
		g_strdup_printf( "%s\\0%s", left_token, right_token );

	str = g_regex_replace( regex, raw_str, -1, 0, replacement, 0, NULL );

	return str;
}

/* Append a label containing a string to a given GtkBox.
 *
 * data - The string.
 *
 * user_data - The GtkBox.
 *
 */
static void
search_append_string( gpointer data, gpointer user_data )
{
	GtkWidget *label, *listbox;
	gchar *str;

	str = (gchar *) data;

	label = gtk_label_new( str );

	listbox = GTK_WIDGET( user_data );

	gtk_box_append( GTK_BOX( listbox ), label );
}

/* Append a label containing a string with marked up matching substrings.
 */
static void
search_append_match_info( gpointer data, gpointer user_data )
{
	const gchar *text;
	const gchar *markup;
	GMatchInfo *match_info;
	GtkWidget *box, *label, *listbox;
	gboolean regex_enabled;

	match_info = (GMatchInfo *) data;

	text = g_match_info_get_string( match_info );

	box = GTK_WIDGET( user_data );

	listbox = gtk_widget_get_last_child( box );

	label = gtk_label_new( NULL );

	markup = get_markup_from_match_info( match_info );

	gtk_label_set_markup( GTK_LABEL( label ), markup );

	gtk_box_append( GTK_BOX( listbox ), label );
}

/* Create the initial "listbox", a GtkBox that contains the searchable strings.
 */
static GtkWidget *
create_initial_listbox()
{
	GtkWidget *initial_listbox;
	GList *strings;

	initial_listbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	strings = get_strings();

	g_list_foreach( strings, search_append_string, initial_listbox );

	return initial_listbox;
}

/* Create a new "filtered listbox" - a GtkBox that only contains strings from 
 * get_strings that contain the substring @text.
 *
 * text - the substring to search for in the strings returned by get_strings
 *
 */
static GtkWidget *
create_filtered_listbox( gchar *text, GtkWidget *box )
{
	GtkWidget *search_bar, *search_bar_box, *check_button, *listbox;
	GList *strings, *found;
	gboolean regex_enabled;

	search_bar = gtk_widget_get_first_child( box );

	search_bar_box = gtk_search_bar_get_child(
		GTK_SEARCH_BAR( search_bar ) );

	check_button = gtk_widget_get_last_child( search_bar_box );

	regex_enabled = gtk_check_button_get_active(
		GTK_CHECK_BUTTON( check_button ) );

	listbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_box_append( GTK_BOX( box ), listbox );

	strings = get_strings();

	found = find_strings_with_substring( strings, (gchar *) text,
		regex_enabled );

	g_list_foreach( found, search_append_match_info, box );

	return listbox;
}

/* This is the callback function called whenever the GtkSearchEntry is modified
 * by the user.
 */
static void
search_changed( GtkWidget *search, gpointer user_data )
{
	GtkWidget *search_bar, *search_box, *search_entry, *box, *listbox,
	 	*label;
	GList *strings, *found;
	const gchar *text;

	box = GTK_WIDGET( user_data );

	search_bar = gtk_widget_get_first_child( box );

	search_box = gtk_search_bar_get_child(
		GTK_SEARCH_BAR( search_bar ) );
	
	search_entry = gtk_widget_get_first_child( search_box );;

	listbox = gtk_widget_get_last_child( box );

	gtk_box_remove( GTK_BOX( box ), listbox );

	text = gtk_editable_get_text( GTK_EDITABLE( search_entry ) );

	listbox = create_filtered_listbox( (gchar *) text, box );
}

/* Activate the GtkApplication. This is the callback function for the "activate"
 * signal.
 */
static void
activate( GtkApplication *app, gpointer user_data )
{
	GtkWidget *window, *box, *search_bar_box, *search_bar, *search_entry, *initial_listbox,
		*check_button_label, *check_button;

	GList *strings;

	window = gtk_application_window_new( app );

	box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_window_set_child( GTK_WINDOW( window ), box );

	search_bar = gtk_search_bar_new();


	gtk_search_bar_set_search_mode( GTK_SEARCH_BAR( search_bar), TRUE );
	
	search_bar_box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );

	search_entry = gtk_search_entry_new();

	gtk_box_append( GTK_BOX( search_bar_box ), search_entry );

	gtk_search_bar_set_child( GTK_SEARCH_BAR( search_bar), search_bar_box );

	gtk_search_bar_connect_entry( GTK_SEARCH_BAR( search_bar),
		GTK_EDITABLE( search_entry ) );

	g_signal_connect( search_entry, "search-changed",
		G_CALLBACK( search_changed ), box );

	gtk_box_append( GTK_BOX( box ), search_bar );

	check_button_label = gtk_label_new( NULL );

	gtk_label_set_markup( GTK_LABEL( check_button_label ),
		"<b>.*</b>" );

	gtk_box_append( GTK_BOX( search_bar_box ), check_button_label );

	check_button = gtk_check_button_new();

	g_signal_connect( check_button, "toggled",
		G_CALLBACK( search_changed ), box );

	gtk_box_append( GTK_BOX( search_bar_box ), check_button );

	initial_listbox = create_initial_listbox();

	gtk_box_append( GTK_BOX( box ), initial_listbox );

	gtk_widget_show( window );
}

/* Set the "activate" signal handler and wait for the app to become ready.
 */
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
