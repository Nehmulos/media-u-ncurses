#include <curl/curl.h>
#include <ncurses.h>
#include <string.h>
#include "json.h"
#include "TvShows.h"
#include "CurlResult.h"


void TvShows_init(struct TvShows* this) {
    this->lists = List_create();
}

void TvShows_destroyMembers(struct TvShows* this) {
    List_destroyWithContent(this->lists, TvShowList);
}

DEFAULT_CREATE_DESTROY(TvShows)

void TvShows_fetch(struct TvShows* this, const char* baseUrl) {
    CURL* handle = curl_easy_init();
    struct CurlResult userdata;
    CurlResult_init(&userdata);
    char url[256];
    sprintf(url, "%s/%s", baseUrl, "api/library/filter/lists");
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &userdata);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlResult_write);
    curl_easy_perform(handle);
    json_value* json = CurlResult_parse(&userdata);
    TvShows_restore(this, json);
}

void TvShows_restore(struct TvShows* this, json_value* json) {
    if (!json || json->type != json_object) {
	printw("failed to parse json\n");
	refresh();
	return;
    }
    int length = json->u.object.length;
    for (int i=0; i < length; ++i) {
	char* itemName = json->u.object.values[i].name;
	if (0 == strcmp(itemName, "lists")) {
	    json_value* lists = json->u.object.values[i].value;
	    if (lists->type != json_object) {
		continue;
	    }
	    int listsLength = lists->u.object.length;
	    for (int j=0; j < listsLength; ++j) {
		char* name = lists->u.object.values[j].name;
		json_value* list = lists->u.object.values[j].value;
		TvShows_restoreList(this, name, list);
	    }
	}
    }
}

void TvShows_restoreList(struct TvShows* this, const char* name, json_value* json) {
    printw("list, %s\n", name);
    if (json->type != json_array) {
	return;
    }
    struct TvShowList* list = TvShowList_create();
    bool hasLis = false;
    int length = json->u.array.length;
    for (int i=0; i < length; ++i) {
	json_value* value = json->u.array.values[i];
	struct TvShowLi* li = TvShowLi_restore(value);
	if (li) {
	    hasLis = true;
	    List_pushBack(list->lis, li);
	}
    }
    if (hasLis) {
	List_pushBack(this->lists, list);
    } else {
	free(list);
    }
}

struct TvShowLi* TvShowLi_restore(json_value* json) {
    if (json->type != json_object) {
	return NULL;
    }
    struct TvShowLi* li = malloc(sizeof(struct TvShowLi));
    int showLength = json->u.object.length;
    for (int i=0; i < showLength; ++i) {
	char* key = json->u.object.values[i].name;
	json_value* value = json->u.object.values[i].value;
	if (0 == strcmp(key, "name") && value->type == json_string) {
	    int strLength = value->u.string.length;
	    li->name = malloc(sizeof(char)*strLength);
	    strcpy(li->name, value->u.string.ptr);
	    printw("added show %s\n", li->name);
	} else if (0 == strcmp(key, "totalEpisodes") && value->type == json_integer) {
	    li->totalEpisodes = value->u.integer;
	} else if (0 == strcmp(key, "watchedEpisodes") && value->type == json_integer) {
	    li->watchedEpisodes = value->u.integer;
	}
    }
    return li;
}


void TvShowList_init(struct TvShowList* this) {
    this->name = NULL;
    this->lis = List_create();
}

void TvShowList_destroyMembers(struct TvShowList* this) {
    List_destroyWithContent(this->lis, TvShowLi);
}

DEFAULT_CREATE_DESTROY(TvShowList)

void TvShowLi_init(struct TvShowLi* this) {
    this->name = NULL;
    this->watchedEpisodes = 0;
    this->totalEpisodes = 0;
}

void TvShowLi_destroyMembers(struct TvShowLi* this) {
    if (this->name) {
	free(this->name);
    }
}

DEFAULT_CREATE_DESTROY(TvShowLi)
