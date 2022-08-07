#define _GNU_SOURCE
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <libxml2/libxml/xpath.h>

typedef struct Feed {
  char* url;
  char* data;
  int length;
} Feed;

#define Stopif(assertion, action, message, ...) { \
  if (assertion) { \
    if (message) { \
      fprintf(stderr, message, ##__VA_ARGS__); fprintf(stderr, "\n"); \
    } \
    {action;} \
  } \
}

char* RSS_URL = "https://medium.com/feed/tag/";
char* OUTPUT = "index.html";

void printToHTML(xmlXPathObjectPtr urls, xmlXPathObjectPtr titles)
{
  FILE* file = fopen(OUTPUT, "w");
  for (int i = 0; i < titles->nodesetval->nodeNr; i++)
  {
    xmlChar* url = xmlNodeGetContent(urls->nodesetval->nodeTab[i]);
    xmlChar* title = xmlNodeGetContent(titles->nodesetval->nodeTab[i]);

    fprintf(file, "<a href=\"%s\">%s</a><br>", url, title);

    xmlFree(url);
    xmlFree(title);
  }

  fclose(file);
}

int parse(Feed* feed)
{
  xmlChar const* titlePath = (xmlChar*)"//item/title";
  xmlChar const* linkPath = (xmlChar*)"//item/link";

  xmlDocPtr doc = xmlReadMemory(feed->data, feed->length, feed->url, 0, 0);
  Stopif(!doc, return -1, "Error: Unable to parse feed");

  xmlXPathContextPtr context = xmlXPathNewContext(doc);
  Stopif(!context, return -2, "Error: Unable to create new xpath context");

  xmlXPathObjectPtr titles = xmlXPathEvalExpression(titlePath, context);
  xmlXPathObjectPtr links = xmlXPathEvalExpression(linkPath, context);

  Stopif(!titles || !links, return -3, "Error: unable to find xml nodes");

  printToHTML(links, titles);

  xmlXPathFreeObject(titles);
  xmlXPathFreeObject(links);
  xmlXPathFreeContext(context);
  xmlFreeDoc(doc);

  return 0;
}

size_t curlCallback(void *data, size_t size, size_t nmemb, void *userp)
 {
   Feed *feed = (Feed*)userp;

   size_t totalSize = size * nmemb;
   char *ptr = realloc(feed->data, feed->length + totalSize + 1);
   if(ptr == 0)
   {
     return 0;
   }

   feed->data = ptr;
   memcpy(&(feed->data[feed->length]), data, totalSize);
   feed->length += totalSize;
   feed->data[feed->length] = 0; // null-terminator

   return totalSize;
 }

#define FETCH_FAILED ((void *) -1)

Feed* fetchRSS(char* url)
{
  CURL* curl = curl_easy_init();
  Stopif(!curl, return FETCH_FAILED, 0);

  Feed* feed = malloc(sizeof(Feed));
  *feed = (Feed){.url = url};

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, feed);

  CURLcode result = curl_easy_perform(curl);
  Stopif(result != CURLE_OK, return FETCH_FAILED, 0);

  curl_easy_cleanup(curl);
  return feed;
}

void feedFree(Feed* feed)
{
  free(feed->data);
  free(feed->url);
  free(feed);
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: prog <medium rss tag>\n");
    exit(EXIT_FAILURE);
  }

  char* feedURL = 0;
  asprintf(&feedURL, "%s%s", RSS_URL, argv[1]);

  Feed* feed = fetchRSS(feedURL);

  Stopif(feed == FETCH_FAILED, free(feedURL); return 1, "failed to download feed for %s", argv[1]);

  int result = parse(feed);
  feedFree(feed);

  return result;
}
