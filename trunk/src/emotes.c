
#include <gnome.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "emotes.h"
#include <glib.h>

EmoteSet *emote_set = NULL;

static int xml_parse_emo(const char *basedir);

gboolean emote_load_set(const gchar *name)
{
	gchar *file;
	gchar *path;

	/* usun stary zestaw */
	if(emote_set != NULL)
		emote_unload_set();

	emote_set = (EmoteSet *)g_malloc(sizeof(EmoteSet));
	emote_set->emotes = NULL; //g_slist_alloc();
	
  	path = gnome_program_locate_file(NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
		"glen/emotes/", FALSE, NULL);
	file = g_strconcat(path, name, "/", NULL);

	g_free(path);

	if(xml_parse_emo(file) != 0) {
		g_free(emote_set);
		emote_set = NULL;
		puts("Blad w emo.xml");
	}

	g_free(file);

	return (emote_set != NULL);
}

void emote_unload_set()
{
	GSList *l, *b;
	Emote *em;
	gchar *tag;

	if(emote_set == NULL)
		return;

	l = emote_set->emotes;

	while(l) {
		em = (Emote *)l->data;

		if(em != NULL) {
			b = em->tags;

			while(b) {
				tag = (gchar *)b->data;
				if(tag != NULL)
					g_free(tag);
				b = b->next;
			}

			g_slist_free(em->tags);
			g_free(em->tooltip);
			g_free(em->data);
			g_free(em->url);
			g_free(em);
		}

		l = l->next;
	}

	g_slist_free(emote_set->emotes);
	g_free(emote_set->template);
	g_free(emote_set);
	emote_set = NULL;
}

static int xml_parse_emo(const char *filebase)
{
/* TODO: ladniej to zrobic... */
	xmlNode *cur_node, *child_node, *root;
	xmlDocPtr doc;
	FILE *fp;
	gsize len;
	char *buf, *tmp;
	Emote *em;
	int ret;
	gchar *filename;
	
	ret = 0;

	filename = g_strconcat(filebase, "emo-utf.xml", NULL);

	fp = fopen(filename, "r");
	if(fp == NULL) {
		perror(filename);
		return 1;
	}

	g_free(filename);

	/* po co nam stat ;) */
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buf = g_malloc(len+1);
	fgets(buf, len, fp);

	doc = xmlParseMemory(buf, strlen(buf));
	if(doc == NULL) {
		g_free(buf);
		printf("doc == NULL 1\n");
		return -1;
	}

	root = xmlDocGetRootElement(doc);

	if(root == NULL || root->name == NULL) {
		xmlFreeDoc(doc);
		g_free(buf);
		return 1;
	}

	tmp = xmlNodeGetContent(root);
	emote_set->template = g_strdup(tmp);
	xmlFree(tmp);

	xmlFreeDoc(doc);
	memset(buf, 0, len+1);
	fread(buf, len, 1, fp);
	fclose(fp);

	doc = xmlParseMemory(buf, len);
	if(doc == NULL) {
		g_free(buf);
		g_free(emote_set);
		return 1;
	}

	root = xmlDocGetRootElement(doc);
	if(root == NULL || root->name == NULL ||
			xmlStrcmp(root->name, "defs") != 0) {
		xmlFreeDoc(doc);
		g_free(buf);
		puts("NULL 2");
		return 1;
	}

	for(cur_node = root->children; cur_node != NULL;cur_node =
			cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE ||
			xmlStrcmp(cur_node->name, "i") != 0) {
			if(xmlStrcmp(cur_node->name, "text") == 0)
				continue;
			else {
				ret = 1;
				break;
			}
		}

		tmp = xmlGetProp(cur_node, "g");
		/* emotka bez nazwy pliku, idziemy dalej */
		if(tmp == NULL)
			continue;

		filename = g_strconcat(filebase, tmp, NULL);

		fp = fopen(filename, "r");
		g_free(filename);
		if(fp == NULL) {
			g_warning("Blad przy ladowaniu emotki %s", tmp);
			continue;
		}
		
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
			
		em = (Emote *)g_malloc(sizeof(Emote));
		
		em->data = (gchar *)g_malloc(len);
		fread(em->data, len, 1, fp);
		fclose(fp);

		em->size = len;
		em->url = tmp;
		
		tmp = xmlGetProp(cur_node, "d");
		if(tmp == NULL)
			em->tooltip = g_strdup("");
		else
			em->tooltip = g_strdup(tmp);

		em->tags = NULL; //g_slist_alloc();

		for(child_node = cur_node->children; child_node != NULL;
			child_node = child_node->next) {
			if(child_node->type == XML_ELEMENT_NODE) {
				if(xmlStrcmp(child_node->name, "t") == 0) {
					tmp = xmlNodeGetContent(child_node);
					if(strlen(tmp) == 0) continue;
/*
					printf("I: '%s', '%s', '%s'\n", em->url,
						tmp,
						g_markup_escape_text(tmp, -1));
*/
					em->tags = g_slist_append(em->tags,
						g_markup_escape_text(tmp,
								-1));
				}
			}
		}

		emote_set->emotes = g_slist_append(emote_set->emotes, em);
	}

	xmlFreeDoc(doc);
	
	g_free(buf);

	return ret;
}

Emote * emote_get_by_url(const gchar *url)
{
	GSList *l;
	Emote *e;

	l = emote_set->emotes;

	while(l != NULL) {
		e = (Emote *)l->data;

		if(e != NULL && g_strcasecmp(url, e->url) == 0)
			return e;

		l = l->next;
	}

	return NULL;
}

