#include "server.h"
#include "wrapper.h"
#include "parsing.h"

#include <stdarg.h>
#include <string.h>

serverFlag* serverFlag_new(char* flag_name, char* flag_value) {
	serverFlag* new_flag = xmalloc(sizeof(serverFlag));

	new_flag->flag_name = xmalloc(ustrlen(flag_name) + 1);
	strcpy(new_flag->flag_name, flag_name);

	new_flag->flag_value = xmalloc(ustrlen(flag_value) + 1);
	strcpy(new_flag->flag_value, flag_value);

	new_flag->subflags = NULL;
	
	return new_flag;
}

void serverFlag_free(serverFlag* flag) {
	free(flag->flag_name);
	free(flag->flag_value);

	if (flag->subflags != NULL) {
		serverFlag* head = flag->subflags;

		while (head->pnext_flag != NULL) {
			head = head->pnext_flag;
			serverFlag_free(head);
		}

        serverFlag_free(head);
	}

	free(flag);
}

void serverFlag_add_subflag(serverFlag* parent_flag, serverFlag* subflag) {
	serverFlag* head = parent_flag->subflags;
	while (head != NULL) {
		head = head->subflags;
	}
	head->subflags = subflag;
}


serverFeed* serverFeed_new(serverFeed* parent_feed, serverFlag* flags) {
    serverFeed* new_feed = xmalloc(sizeof(serverFeed));
    new_feed->client = NULL;
    new_feed->parent_feed = parent_feed;
    new_feed->flags = flags;
    new_feed->subfeeds = NULL;
    
	return new_feed;
}

void serverFeed_free(serverFeed* feed) {
    // note: this does not free the client associated to the feed
	serverFlag* head = feed->flags;
	while (head != NULL) {
		head = head->pnext_flag;
		serverFlag_free(head);
	}

	serverFeed* head_feed = feed->subfeeds;
	while (head_feed != NULL) {
		head_feed = head_feed->pnext_serverfeed;
		serverFeed_free(head_feed);
	}

	free(feed);
}

void serverFeed_add_subfeed(serverFeed* parent_feed, serverFeed* child_feed) {
	serverFeed* head = parent_feed->subfeeds;
	while (head != NULL) {
		head = head->pnext_serverfeed;
	}
	head->pnext_serverfeed = child_feed;
}

serverFlag* serverFlag_get_flag(serverFlag* root_flag, char* flag_name) {
	serverFlag* head = root_flag->subflags;
	while (head != NULL) {
		head = head->pnext_flag;
	}
	return head->pnext_flag;
}

serverFlag* serverFlag_get_byaddr(serverFlag* root_flag, char* addr) {
    char* dot = strchr(addr, '.');
    if (dot != NULL) {
        dot++;
        char* new_root = strchr(dot, '.');
        *new_root = '\0';
        return serverFlag_get_flag(root_flag, new_root);
    } else {
        return serverFlag_get_flag(root_flag, addr);
    }
}

serverFeed* serverFeed_new(serverFeed* root_feed, char* feed_name) {
    
}
