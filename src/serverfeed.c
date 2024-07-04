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

		while (head != NULL) {
			serverFlag* tmp = head->pnext_flag;
			serverFlag_free(head);
            head = tmp;
		}

	}

	free(flag);
}

void serverFlag_add_subflag(serverFlag* parent_flag, serverFlag* subflag) {
	serverFlag* head = parent_flag->subflags;
    if (head != NULL) {
        while (head->pnext_flag != NULL) {
            head = head->pnext_flag;
        }
        head->pnext_flag = subflag;
    } else {
        parent_flag->subflags = subflag;
    }
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
		serverFlag* tmp = head->pnext_flag;
		serverFlag_free(head);
        head = tmp;
	}

	serverFeed* head_feed = feed->subfeeds;
	while (head_feed != NULL) {
		serverFeed* tmp = head_feed->pnext_serverfeed;
		serverFeed_free(head_feed);
        head_feed = tmp;
	}

	free(feed);
}

void serverFeed_add_subfeed(serverFeed* parent_feed, serverFeed* child_feed) {
	serverFeed* head = parent_feed->subfeeds;
    if (head != NULL) {
        while (head->pnext_serverfeed != NULL) {
            head = head->pnext_serverfeed;
        }
        head->pnext_serverfeed = child_feed;
    } else {
        parent_feed->subfeeds = child_feed;
    }
}

serverFlag* serverFlag_get_flag(serverFlag* root_flag, char* flag_name) {
	serverFlag* head = root_flag->subflags;
	while (head->pnext_flag != NULL) {
        if (strcmp(head->flag_name, flag_name) == 0) {
            return head;
        }
		head = head->pnext_flag;
	}
	return NULL;
}

serverFlag* serverFlag_get_byaddr(serverFlag* root_flag, char* addr) {

    // this way we leave the original address unmodified
    char addr_copy[strlen(addr) + 1];
    strcpy(addr_copy, addr);
    addr = addr_copy;

    char* dot = strchr(addr, '.');
    if (dot != NULL) {
        dot++;
        char* new_root_name = strchr(dot, '.');

        // get the current address level
        // eg: if we are at level 2 of foo.bar.bazz, return "bar"
        // if we're at the terminal level, then there's no need to crop the new root
        if (new_root_name != NULL) {
            *new_root_name = '\0';
        }

        serverFlag* new_root = serverFlag_get_flag(root_flag, new_root_name);

        // if we're not at the terminal level, add back the dot
        if (new_root_name != NULL) {
            *new_root_name = '.';
        }

        return serverFlag_get_flag(new_root, dot);
    } else {
        return serverFlag_get_flag(root_flag, addr);
    }
}
