# Threadpool Implementation
> Our implementation follows the idea in [this stackflow post](https://stackoverflow.com/a/32593825).

The Threadpool class mainly uses two data structures to distribute tasks to a pool of threads.
In detail, a `vector<thread>` containing `num_threads` `std::thread` is created in the constructor. A `vector<function>` is used as a queue where producers push tasks and consumers grab tasks.
Each thread is a consumer, which runs the `ThreadLoop()` function to constantly check if the job queue has jobs, and retrieve one if there are.
Other programs that instantiate the `Threadpool` class act as producers and can send tasks to the Threadpool using its `add_job` method which just pushes a job, i.e. `function`, to the queue and notifies a thread.
Since it uses the `producer-consumer` model, a lock is required to ensure the mutual exclusion. We also use a `condition_variable` to avoid the busy wait, improving the efficiency.

# Communication Pipelines
> The store async server part borrows lots of code from https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_server.cc.
> The vendor_client borrows lots of code from https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_client.cc.
Our store has two roles, server for the users and client for the vendors. Hence two communication subroutines are needed in serving a product bid request.
First, our store assemble an async server by binding the address and port, registering the service, and constructing a completion queue. Then it calls `HandleRpcs` as a main loop to handle requests.
Each request is handled by a `CallData` instance which implements a state machine consisting of three states: CREATE, PROCESS and FINISH.
1. Each `CallData` is initialized with the `CREATE` state.
2. The constructor calls `Proceed` to change the state to `PROCESS` to indicate that it's ready to take a request by adding itself into the completion queue.
3. When the client issues a request, the main loop will be unblocked from `completion_queue->Next`, retrieve the request from the queue, and call `Proceed` to handle the request.
4. The `Proceed` calls `add_job` to submit the request handling logic, which is wrapped in a function, to the thread pool.
5. It will first create a new `CallData` instance to serve new clients while the current one is processed and then creates a vendor client to query product bids from vendors(The detail of this communication subroutine is elaborated later).
6. After collecting all the product data from vendors, mark the status as `FINISH` and returns the response.

The vendor client receives a list of vendor ip addresses and creates stubs for each vendor. Then we iterate stubs and issue asynchronous request to the vendors by calling its `PrepareAsyncgetProductBid`.
Finally, we wait on the `completion_queue.Next` for all the requests to finish and return the result.