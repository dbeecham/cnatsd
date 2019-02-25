# cnatsd

cnatsd needs to be able to do

* A client connects to the server
  + Nothing interesting here, cnatsd just needs to handle TCP connections.
* A client subscribes to a topic
  + cnatsd need to keep track of subscriptions for each user
* A client publishes a message to a topic
  + cnatsd need to efficiently find all subscribers to a topic
* A client unsubscribes to a topic (by sid)
  + cnatsd needs a sid -> subscription mapping for each user
* A client disconnects



Client = (SID -> Topic) * Parse
cnatsd = (FD -> Client) * (Topic -> [Client])


* When a client unsubscribes
    + We know the FD; find the client by (FD -> Client) function
    + We know the SID; find the topic by (SID -> Topic) function
    + Remove this client from the (Topic -> [Client]) function
    + Remove this SID from the (SID -> Topic) function

