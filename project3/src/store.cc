#include "threadpool.h"

#include <iostream>

#include <grpc++/grpc++.h>

#include "store.grpc.pb.h"
#include "vendor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using store::ProductQuery;
using store::ProductReply;
using store::ProductInfo;
using vendor::BidQuery;
using vendor::BidReply;

class StoreService final : public Store::Service { 
	public:
		Status getProducts(ServerContext* context, const ProductQuery* query, ProductReply* reply) override {
			std::string product_name = query->product_name();
		
			std::cout << "Received request for product: " << product_name << std::endl;
		
			BidQuery bid_query;
			bid_query.set_product_name(product_name);
		
			BidReply bid_reply;
		
			Status status = stub_->getProductBid(&context, bid_query, &bid_reply);
		
			if (!status.ok()) {
				std::cout << status.error_code() << ": " << status.error_message()
						<< std::endl;
				return status;
			}
		
			std::cout << "Received bid from vendor: " << bid_reply.vendor_id() << " with price: " << bid_reply.price() << std::endl;
		
			ProductInfo* product_info = reply->add_products();
			product_info->set_price(bid_reply.price());
			product_info->set_vendor_id(bid_reply.vendor_id());
		
			return Status::OK;
		}

  	private:
		std::unique_ptr<Vendor::Stub> stub_;
};

void run_store_server(const std::string vendor_addresses_file, const std::string server_address, const int num_max_threads) {
	StoreService(vendor_addresses_file, max_threads);
	
	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;
	
	server->Wait();
}

int main(int argc, char** argv) {
	// Your server should be able to accept `command line input` of 
	// the vendor addresses file,  
	// `address` on which it is going to expose its service 
	// and `maximum number of threads` its threadpool should have.
	int num_max_threads;
	std::string server_address;
	std::string vendor_addresses_file;
	if (argc == 4) {
		vendor_addresses_file = std::string(argv[1]);
		server_address = std::string(argv[2]);
		num_max_threads = std::min(1, atoi(argv[3]));
	}
	else {
		std::cerr << "Correct usage: ./store <filepath for vendor addresses> \
               <ip address:port to listen on for clients> \
               <maximum number of threads in threadpool>" << std::endl;
		return EXIT_FAILURE;
	}

	run_store_server(vendor_addresses_file, server_address, num_max_threads);

	return EXIT_SUCCESS;
}

