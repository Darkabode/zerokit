#ifndef __ELOCKER_SOCKET_H_
#define __ELOCKER_SOCKET_H_

/**
    A wrapper for a streaming (TCP) socket.

    This allows low-level use of sockets; for an easier-to-use messaging layer on top of
    sockets, you could also try the InterprocessConnection class.

    @see DatagramSocket, InterprocessConnection, InterprocessConnectionServer
*/
class StreamingSocket
{
public:
    /** Creates an uninitialised socket.

        To connect it, use the connect() method, after which you can read() or write()
        to it.

        To wait for other sockets to connect to this one, the createListener() method
        enters "listener" mode, and can be used to spawn new sockets for each connection
        that comes along.
    */
    StreamingSocket();

    /** Destructor. */
    ~StreamingSocket();

    static bool initSockets();
    static void doneSockets();

    /** Tries to connect the socket to hostname:port.

        If timeOutMillisecs is 0, then this method will block until the operating system
        rejects the connection (which could take a long time).

        @returns true if it succeeds.
        @see isConnected
    */
    bool connect (const zgui::String& remoteHostname, int remotePortNumber, int timeOutMillisecs = 7000);

    /** True if the socket is currently connected. */
    bool isConnected() const throw() { return connected; }

    /** Closes the connection. */
    void close();

    /** Returns the name of the currently connected host. */
    const zgui::String& getHostName() const throw() { return hostName; }

    /** Returns the port number that's currently open. */
    int getPort() const throw() { return portNumber; }

    /** Waits until the socket is ready for reading or writing.

        If readyForReading is true, it will wait until the socket is ready for
        reading; if false, it will wait until it's ready for writing.

        If the timeout is < 0, it will wait forever, or else will give up after
        the specified time.

        If the socket is ready on return, this returns 1. If it times-out before
        the socket becomes ready, it returns 0. If an error occurs, it returns -1.
    */
    int waitUntilReady(bool readyForReading, int timeoutMsecs) const;

    /** Reads bytes from the socket.

        If blockUntilSpecifiedAmountHasArrived is true, the method will block until
        maxBytesToRead bytes have been read, (or until an error occurs). If this
        flag is false, the method will return as much data as is currently available
        without blocking.

        @returns the number of bytes read, or -1 if there was an error.
        @see waitUntilReady
    */
    int read(void* destBuffer, int maxBytesToRead, bool blockUntilSpecifiedAmountHasArrived);

    /** Writes bytes to the socket from a buffer.

        Note that this method will block unless you have checked the socket is ready
        for writing before calling it (see the waitUntilReady() method).

        @returns the number of bytes written, or -1 if there was an error.
    */
    int write(const void* sourceBuffer, int numBytesToWrite);

private:
    zgui::String hostName;
    int volatile portNumber, handle;
    bool connected;

    StreamingSocket(const zgui::String& hostname, int portNumber, int handle);

    static bool socketsStarted;
};

#endif // __ELOCKER_SOCKET_H_
