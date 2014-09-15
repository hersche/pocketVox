#include "pocketvox_dictionnary.h"
#include <math.h>

enum
{
	PROP_0,
	PROP_RAWFILE
};

typedef struct pocketVoxWord 
{
	gchar *word;
	gint occurence;
}pocketVoxWord;

struct _PocketvoxDictionnaryPrivate 
{
	gboolean	loaded;
	gchar		*path;
	
	GHashTable 	*hash;
	GHashTable  *words;
	GHashTable  *tfidf;

	GThreadPool *pool;
};

static GMutex mutex;

G_DEFINE_TYPE (PocketvoxDictionnary, pocketvox_dictionnary, G_TYPE_OBJECT);

static void pocketvox_dictionnary_dispose(GObject *object)
{
	G_OBJECT_CLASS (pocketvox_dictionnary_parent_class)->dispose (object);
}

static void pocketvox_dictionnary_process_request(gpointer data, gpointer user_data)
{
	PocketvoxDictionnary* dictionnary = (PocketvoxDictionnary *)user_data;
	gchar* query = (gchar *)data;

	g_return_if_fail(NULL != dictionnary);
	g_return_if_fail(NULL != query);

	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;
	
	g_mutex_lock(&mutex);
	
	//split the string
	GList* words = g_hash_table_get_values(priv->words);
	GList* cmds  = g_hash_table_get_keys(priv->tfidf);
	GList* tabs  = g_hash_table_get_values(priv->tfidf); 
	
	gint i,j ;
	gint N = g_list_length(words);
	gchar* result = NULL;
	
	gdouble *t = (gdouble *)g_malloc0(N*sizeof(gdouble));
	gdouble dist, mindist;
	
	//for each of the dictionnary
	for(i = 0; i < N ; i++)
	{
		gchar** w = g_strsplit_set(query, " ", -1);
		pocketVoxWord *pwv = (pocketVoxWord *)g_list_nth_data(words, i);
		gdouble iter = 0.0f;
		gdouble tot = 0.0f;
		
		t[i] = log((gdouble)N/(gdouble)pwv->occurence);
	
		while(*w != NULL)
		{
			if(!g_strcmp0(*w, pwv->word)) iter = iter + 1.0f;
			tot = tot + 1.0f;
			w++;
		}
		
		t[i] = t[i] * ((gdouble)iter/(gdouble)tot);
	}
	
	//now find the nearest projection among all tfidf projections 
	for(i = 0; i <g_list_length(cmds); i++)
	{
		dist = 0.0f;
		gdouble* tab = (gdouble *)g_list_nth_data(tabs, i);
		
		for(j = 0; j <N; j++)
		{
			dist += (tab[j] - t[j])*(tab[j] - t[j]);
		}
	
		if(i == 0 || dist < mindist)
		{
			mindist = dist;
			result = (gchar *)g_list_nth_data(cmds, i);
		}
	}
	
	
	g_warning("cmd: %s", result);
	
	g_free(t);
	g_list_free(cmds);
	g_list_free(tabs);
	g_list_free(words);

	g_mutex_unlock(&mutex);
}


static void pocketvox_dictionnary_finalize(GObject *object)
{
	PocketvoxDictionnary *dictionnary= POCKETVOX_DICTIONNARY(object);

	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
		TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	
	
	g_free(priv->path);
	g_hash_table_destroy(priv->hash);
	g_hash_table_destroy(priv->words);
	g_hash_table_destroy(priv->tfidf);
	
	g_thread_pool_free(priv->pool, TRUE, TRUE);
	
	G_OBJECT_CLASS (pocketvox_dictionnary_parent_class)->finalize (object);
}

static void pocketvox_dictionnary_set_property (GObject      *gobject,
														 guint         prop_id,
														 const GValue *value,
														 GParamSpec   *pspec)
{
	PocketvoxDictionnary *dictionnary = POCKETVOX_DICTIONNARY(gobject);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;
	
	switch (prop_id)
	{
		case PROP_RAWFILE:
			priv->path = g_strdup((gchar *)g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
			break;
	}
}

static void pocketvox_dictionnary_get_property (GObject    *gobject,
														 guint       prop_id,
														 GValue     *value,
														 GParamSpec *pspec)
{
	PocketvoxDictionnary *dictionnary = POCKETVOX_DICTIONNARY(gobject);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;
	
	switch (prop_id)
	{
		case PROP_RAWFILE:
			g_value_set_string(value, priv->path);		
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
			break;
	}
}

static void pocketvox_dictionnary_class_init (PocketvoxDictionnaryClass *klass)
{
	g_type_class_add_private (klass, sizeof (PocketvoxDictionnaryPrivate));

	GObjectClass *gklass = G_OBJECT_CLASS(klass);
	gklass->get_property = pocketvox_dictionnary_get_property;
	gklass->set_property = pocketvox_dictionnary_set_property;
	gklass->dispose      = pocketvox_dictionnary_dispose;
	gklass->finalize     = pocketvox_dictionnary_finalize;
	
	GParamSpec *pspec = g_param_spec_string ("rawfile",
		"rawfile",
		"rawfile",
		NULL,
		G_PARAM_READWRITE);
	g_object_class_install_property (gklass, PROP_RAWFILE, pspec);

}

static void pocketvox_dictionnary_free_word(gpointer data)
{
	pocketVoxWord *w =(pocketVoxWord *)data;
	
	g_free(w->word);
	g_free(w);
}

/** Initialisation de l'objet */
static void pocketvox_dictionnary_init (PocketvoxDictionnary *dictionnary){
	g_return_if_fail(NULL != dictionnary);

	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	
	
	priv->hash 	= g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	priv->words = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, pocketvox_dictionnary_free_word);
	priv->tfidf = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	
	priv->pool = g_thread_pool_new(pocketvox_dictionnary_process_request, dictionnary, 2, FALSE, NULL);
	
	priv->loaded = FALSE;
}

static gboolean pocketvox_dictionnary_find_word(gpointer key, gpointer value, gpointer user_data)
{
	pocketVoxWord* w = (pocketVoxWord *)user_data;
	
	return !g_strcmp0(w->word, (gchar *)key);
}

static void pocketvox_dictionnary_load_raw(PocketvoxDictionnary *dictionnary)
{
	g_return_if_fail(NULL != dictionnary);

	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;		
	
	g_return_if_fail(g_file_test(priv->path, G_FILE_TEST_EXISTS));
	
	GIOChannel *io = g_io_channel_new_file(priv->path, "r", NULL);
	
	gchar *line;
	
	do
	{
		g_io_channel_read_line(io, &line, NULL, NULL, NULL);
		
		if(line != NULL)
		{
			gchar** w;
			gchar** fields = g_strsplit_set(line, "=\n", 3);
			g_hash_table_insert(priv->hash, g_strdup(fields[0]), g_strdup(fields[1]));
			
			//add new word to the dictionnary word
			
			w = g_strsplit_set(fields[0], " ", -1);
			
			while( *w != NULL)
			{
				pocketVoxWord* nw = (pocketVoxWord *)g_malloc0(sizeof(pocketVoxWord));
				nw->word = g_strdup(*w);
				nw->occurence = 1;

				pocketVoxWord* ww = (pocketVoxWord *)g_hash_table_find(priv->words, pocketvox_dictionnary_find_word, nw);
				
				if (ww != NULL)
				{
					ww->occurence ++;
					
					g_free(nw->word);
					g_free(nw);
				}
				else
				{
					g_hash_table_insert(priv->words,g_strdup(*w), nw);	
				}
				
				w++;
			}

			g_strfreev(fields);
		}
		
	}while( line != NULL);
	
}

static void pocketvox_dictionnary_tfidf(PocketvoxDictionnary *dictionnary)
{
	g_return_if_fail(NULL != dictionnary);

	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	
	
	GList *keys 	= g_hash_table_get_keys(priv->hash);
	GList *commands = g_hash_table_get_values(priv->hash);
	GList *nwords	= g_hash_table_get_values(priv->words);
	gint i, j;
	gint N = g_list_length(keys);
		 
	for(i = 0; i<N; i++)
	{
		gchar *key   = (gchar *)g_list_nth_data(keys, i);
		gchar *cmd   = (gchar *)g_list_nth_data(commands, i);

		gdouble *tab = (gdouble *)g_malloc0(g_hash_table_size(priv->words)*sizeof(gdouble));

		//get the tfidf frequency
		for(j = 0; j < g_list_length(nwords); j++ )
		{
			gdouble iter = 0.0;
			gdouble tot = 0.0;
			pocketVoxWord* pw = (pocketVoxWord *)g_list_nth_data(nwords, j);
			
			tab[j] = log((gdouble)N/(gdouble)pw->occurence);
			gchar **ww = g_strsplit_set(key, " ", -1);
						
			while( *ww != NULL)
			{
				//g_print("%s %s\n", *ww, pw->word);
				if(!g_strcmp0(*ww, pw->word)) iter = iter + 1.0f;
				
				tot = tot + 1.0f;
				ww++;
			}
			
			tab[j] = tab[j] * (gdouble)iter/(gdouble)tot;
		}

		//add it to the hashtable 
		g_hash_table_insert(priv->tfidf, g_strdup(cmd), tab);		
	} 
	
	g_list_free(nwords);
	g_list_free(keys);
	g_list_free(commands);
}


PocketvoxDictionnary* pocketvox_dictionnary_new(gchar* filepath, gboolean load_tfidf)
{
	PocketvoxDictionnary* dictionnary = (PocketvoxDictionnary *)g_object_new(TYPE_POCKETVOX_DICTIONNARY,
																			"rawfile", filepath,
																			NULL);
	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	
	
	priv->path = g_strdup(filepath);
	
	if(load_tfidf == FALSE)
	{
		pocketvox_dictionnary_load_raw(dictionnary);
		pocketvox_dictionnary_tfidf(dictionnary);
	}
	else
	{
		pocketvox_dictionnary_load_tfidf_file(dictionnary, priv->path);
	}
	
	priv->loaded = TRUE;
	
	return dictionnary;
}


void pocketvox_dictionnary_display(PocketvoxDictionnary* dictionnary)
{
	g_return_if_fail(NULL != dictionnary);
	
	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;

	//display all words and frequency in the document
	g_return_if_fail(NULL != priv->words);
	
	GList *words = g_hash_table_get_values(priv->words);
	GList *keys	 = g_hash_table_get_keys(priv->hash);
	GList *commands= g_hash_table_get_values(priv->hash);
	
	gint N = g_list_length(words);
	gint i, j;

	//display all association
	for(i= 0; i< g_list_length(keys); i++)
	{
		gchar *k=(gchar *)g_list_nth_data(keys, i);
		gchar *c=(gchar *)g_list_nth_data(commands, i);
		gdouble* tab = (gdouble *)g_hash_table_lookup(priv->tfidf, c);
		g_print("%s => %s\n\n", k,c);
		
		for(j=0; j < N; j++)
		{
			g_print("%.10f ", tab[j]);
		}
		g_print("\n\n");
	}
		
	//display word frequency	
	for(i = 0; i<N; i++)
	{
		pocketVoxWord *pvw = (pocketVoxWord *)g_list_nth_data(words, i);
		g_print("%s %d\n",pvw->word, pvw->occurence);
	}
	
	g_list_free(words);
	g_list_free(keys);
	g_list_free(commands);
}

void pocketvox_dictionnary_add_new_request(PocketvoxDictionnary *dictionnary, gchar *request)
{
	g_return_if_fail(NULL != dictionnary);
	
	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	

	g_thread_pool_push(priv->pool, request, NULL);
	
	g_usleep(1000);
}

gboolean pocketvox_dictionnary_is_loaded(PocketvoxDictionnary *dictionnary)
{
	g_return_val_if_fail(NULL != dictionnary, FALSE);
	
	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	
	
	return priv->loaded;	
}

void pocketvox_dictionnary_write_tfidf_file(PocketvoxDictionnary *dictionnary)
{
	g_return_if_fail(NULL != dictionnary);
	
	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	
	
	g_return_if_fail(TRUE == priv->loaded);
	
	//build the filename
	gchar* inipath = g_strdup(priv->path);
	
	if(!g_str_has_suffix(priv->path, ".ini"))
	{
		g_free(inipath);
		gchar* dir = g_path_get_dirname(priv->path);
		gchar* base = g_path_get_basename(priv->path);
		gchar** fbase = g_strsplit_set(base, ".", -1);
		
		inipath = g_strdup_printf("%s/%s.ini", dir, fbase[0]); 
		
		g_strfreev(fbase);
		g_free(dir);
		g_free(base);
	}
	
	GKeyFile* keyfile;
    GError *error = NULL;
    gsize size;
    keyfile = g_key_file_new();
    GList *commands = g_hash_table_get_keys(priv->tfidf);
    GList *values = g_hash_table_get_values(priv->tfidf);
    GList *words = g_hash_table_get_values(priv->words);
    gint n = g_hash_table_size(priv->tfidf);
    gint i;
    gchar *data = NULL;
    
    //record firstly the projection
    for(i = 0; i < n; i++)
    {
		gchar* cmd = g_strdup_printf("cmd%d", i);
		gchar* proj = g_strdup_printf("proj%d", i);
		
		gchar* c = (gchar *)g_list_nth_data(commands, i);
		gdouble* tab = (gdouble *)g_list_nth_data(values, i);
				
		g_key_file_set_string(		keyfile, "projection", cmd, c);
		g_key_file_set_double_list(	keyfile, "projection", proj, tab, g_list_length(words));
		
		g_free(cmd);
		g_free(proj);
	}
    
    //then record word frequency
    for(i = 0; i < g_list_length(words); i++)
    {
		pocketVoxWord* w = (pocketVoxWord *)g_list_nth_data(words, i);
		gchar *wcmd = g_strdup_printf("word%d", i);
		gchar *wfreq = g_strdup_printf("freq%d", i);
		
		g_key_file_set_string(keyfile, "words", wcmd, w->word);
		g_key_file_set_integer(keyfile, "words", wfreq, w->occurence);
		g_free(wcmd);
		g_free(wfreq);
	}
    
    data = g_key_file_to_data(keyfile, &size, &error);
    g_file_set_contents(inipath, data, size, &error);
    
    g_free(data);
    g_key_file_free(keyfile);	
    
    g_list_free(words);
    g_list_free(commands);
    g_list_free(values);

    g_free(inipath);
}

void pocketvox_dictionnary_load_tfidf_file(PocketvoxDictionnary* dictionnary, gchar* path)
{
	g_return_if_fail(NULL != dictionnary);
	
	dictionnary->priv = G_TYPE_INSTANCE_GET_PRIVATE (dictionnary,
			TYPE_POCKETVOX_DICTIONNARY, PocketvoxDictionnaryPrivate);
	PocketvoxDictionnaryPrivate *priv = dictionnary->priv;	

	//make the flag loaded to FALSE
	priv->loaded = FALSE;

	g_return_if_fail(g_file_test(path, G_FILE_TEST_EXISTS) == TRUE);
	
	//create the keyfile to parse
	GKeyFile* keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	keyfile = g_key_file_new();
	flags = G_KEY_FILE_KEEP_COMMENTS;
	gint i = 0;
	
	//get all groups in the keyfile
	if(g_key_file_load_from_file (keyfile, path, flags, &error ) )
	{
		gchar** groups = g_key_file_get_groups(keyfile, NULL);
		
		while( *groups != NULL )
		{
			if ( !g_strcmp0(*groups, "projection"))
			{
				gchar** keys = g_key_file_get_keys(keyfile, *groups, NULL, NULL);

				i = 0;
				while(*keys != NULL)
				{
					gchar *cc = g_strdup_printf("cmd%d", i);
					gchar *ct = g_strdup_printf("proj%d", i);
					
					gchar *k  = g_key_file_get_string(keyfile, *groups, cc, NULL);
					gdouble *tab	= g_key_file_get_double_list(keyfile, *groups, ct, NULL, NULL);
					
					g_hash_table_insert(priv->tfidf, g_strdup(k), tab);
					
					i++;
					keys += 2;
					g_free(cc);
					g_free(ct);
				}
			}
			else
			{
				if(!g_strcmp0(*groups,"words"))
				{
					gchar **words = g_key_file_get_keys(keyfile, *groups, NULL, NULL);

					while(*words != NULL)
					{
						pocketVoxWord *pvw = (pocketVoxWord *)g_malloc0(sizeof(pocketVoxWord));
						pvw->word = g_strdup(*words);
						pvw->occurence = (gint)g_key_file_get_integer(keyfile, *groups, *words, NULL);
						
						
						g_hash_table_insert(priv->words, g_strdup(*words), pvw);
						
						words++;
					}	
				}
			}
			
			groups++;
		}
	}
	
	priv->loaded = TRUE;
}
