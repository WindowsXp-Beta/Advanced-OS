#include "threadpool.h"

#include <iostream>

class store { 
	Status getProducts(ServerContext* context, const ProductQuery* query, ProductReply* reply) override {
		string product_name = query->product_name();

		std::cout << "Received request for product: " << product_name << std::endl;

		Bidquery bid_query;
		bid_query.set_product_name(product_name);

		BidReply bid_reply;
		Status status = stub_->getProductBid(&context, bid_query, &bid_reply);

		if (!status.ok()) {
			std::cout << status.error_code() << ": " << status.error_message()
					  << std::endl;
			return false;
		  }

		std::cout << "Received bid from vendor: " << bid_reply.vendor_id() << " with price: " << bid_reply.price() << std::endl;

		ProductInfo* product_info = reply->add_products();
		product_info->set_price(bid_reply.price());
		product_info->set_vendor_id(bid_reply.vendor_id());

		
		return Status::OK;
	}
};

int main(int argc, char** argv) {
	std::cout << "I 'm not ready yet!" << std::endl;
	return EXIT_SUCCESS;
}

