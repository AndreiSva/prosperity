# prosperity
a dead simple yet modern instant messaging server

## Status
Propserity is currently in a very early testing stage, and is currently not functional.

## Main Planned Features

> [!NOTE]  
> None of these features currently exist or are guaranteed to exist.

### Feeds
A Prosperity network is organized as a tree. Something called "Feeds" replace channels. Unlike channels, feeds can have an arbitrary number of subfeeds. When a message is sent to a feed, it is propagated to every subfeed. Likewise, all the messages sent in subfeeds are present and viewable in the parent feed. Users can subscribe to one or multiple subfeeds at once.

### Mandatory TLS
Plaintext connections are pretty bad.

### Built-in authentication
No more `NickServ` hacks. Authentication is built-in to the server using the sqlite library.

### Braindeadly Simple Message Format
Messages are sent in a simple CSV format, making them extremely easy to parse. This also means that it's easy to extend the the server for any purpose.
