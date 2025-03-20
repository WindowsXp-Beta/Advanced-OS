#include "vendor_client.h"

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <utility>
#include <vector>

#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"

using namespace std;

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using vendor::BidQuery;
using vendor::BidReply;
using vendor::Vendor;

VendorClient::VendorClient(vector<string> vendor_ips)
    : vendor_ips_(vendor_ips) {
  for (auto vendorIP : vendor_ips_) {
    shared_ptr<Channel> channel =
        grpc::CreateChannel(vendorIP, grpc::InsecureChannelCredentials());
    stubs_.push_back(Vendor::NewStub(channel));
  }
}

// Assembles the client's payload, sends it and presents the response back
// from the server.
vector<pair<double, string>> VendorClient::getProductBid(string product_name) {
#ifdef DEBUG
  cout << "getProductBid start" << endl;
#endif

  // Data we are sending to the server.
  BidQuery query;
  query.set_product_name(product_name);

  // Container for the data we expect from the server.
  BidReply reply;

  // The producer-consumer queue we use to communicate asynchronously with the
  // gRPC runtime.
  CompletionQueue cq;

  // Storage for the status of the RPC upon completion.
  Status status;

#ifdef DEBUG
  cout << "getProductBid start" << endl;
#endif

  // Since a stub is of type unique pointer, no copy can be created for this.
  // Hence we cannot use index based iterator or every auto based iterators
  // The only solution here is to use explicit iterators
  for (auto itr = stubs_.begin(); itr != stubs_.end(); itr++) {
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

#ifdef DEBUG
    cout << "getProductBid rpc start" << endl;
#endif

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    std::unique_ptr<ClientAsyncResponseReader<BidReply>> rpc(
        (*itr)->PrepareAsyncgetProductBid(&context, query, &cq));

    // StartCall initiates the RPC call
    rpc->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the
    // operation was successful. Tag the request with the integer 1.
    rpc->Finish(&reply, &status, (void*)1);

#ifdef DEBUG
    cout << "getProductBid rpc async done" << endl;
#endif
  }

  // Act upon the status of the actual RPC.
  vector<pair<double, string>> product_bids;

  for (int i = 0; i < vendor_ips_.size(); i++) {
#ifdef DEBUG
    cout << "getProductBid async response" << endl;
#endif

    void* tag;
    bool ok = false;
    // Block until the next result is available in the completion queue "cq".
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or the cq_ is shutting
    // down.
    GPR_ASSERT(cq.Next(&tag, &ok));

    // Verify that the result from "cq" corresponds, by its tag, our previous
    // request.
    GPR_ASSERT(tag == (void*)1);
    // ... and that the request was completed successfully. Note that "ok"
    // corresponds solely to the request for updates introduced by Finish().
    GPR_ASSERT(ok);

    if (status.ok())
      product_bids.emplace_back(reply.price(), reply.vendor_id());
  }

  return product_bids;
}