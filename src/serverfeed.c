#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "server.h"
#include "wrapper.h"
#include "parsing.h"

#define ADDR_SEPARATOR_CHAR '@'

serverFeed* serverFeed_new(serverFeed* parent_feed, char* feed_name) {
    serverFeed* new_feed = xmalloc(sizeof(serverFeed));
    new_feed->feed_name = feed_name;
    new_feed->client = NULL;
    new_feed->parent_feed = parent_feed;
    new_feed->subfeeds = NULL;
    
	return new_feed;
}

void serverFeed_free(serverFeed* feed) {
    // note: this does not free the client associated to the feed

    free(feed->feed_name);

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

serverFeed* serverFeed_get_feed(serverFeed* root_feed, char* feed_name) {
    serverFeed* head = root_feed->subfeeds;
    if (head != NULL) {
        while (head->pnext_serverfeed != NULL) {
            if (strcmp(head->feed_name, feed_name) == 0) {
                return head;
            }
            head = head->pnext_serverfeed;
        }
    }

    return NULL;
}

serverFeed* serverFeed_get_byaddr(serverFeed* root_feed, char* addr) {

    // this way we leave the original address unmodified
    char addr_copy[strlen(addr) + 1];
    strcpy(addr_copy, addr);
    addr = addr_copy;

    char* at = strchr(addr, ADDR_SEPARATOR_CHAR);
    if (at != NULL) {
        at++;
        char* new_root_name = strchr(at, ADDR_SEPARATOR_CHAR);

        // get the current address level
        // eg: if we are at level 2 of foo.bar.bazz, return "bar"
        // if we're at the terminal level, then there's no need to crop the new root
        if (new_root_name != NULL) {
            *new_root_name = '\0';
        }

        serverFeed* new_root = serverFeed_get_feed(root_feed, new_root_name);

        // if we're not at the terminal level, add back the dot
        if (new_root_name != NULL) {
            *new_root_name = ADDR_SEPARATOR_CHAR;
        }

        return serverFeed_get_feed(new_root, at);
    } else {
        return serverFeed_get_feed(root_feed, addr);
    }
}

void serverFeed_send(serverFeed* recipient, CSValue* message) {
    serverFeed* head = recipient->subfeeds;

    FILE* message_file = tmpfile();
    CSValue_put(message_file, message);

    fseek(message_file, 0, SEEK_END);
    size_t message_size = ftell(message_file);
    rewind(message_file);

    char buf[message_size + 1];
    fread(buf, message_size, 1, message_file);
    fclose(message_file);

    size_t written;
    int write_result = SSL_write_ex(recipient->client->ssl_object, buf, message_size, &written);

    while (head != NULL) {
        serverFeed_send(head, message);
        head = head->pnext_serverfeed;
    }
}
