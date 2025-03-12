#include "threadpool.h"

#include <iostream>

class store { 
	Status getProducts(ServerContext* context, const ProductQuery* query, ProductReply* reply) override {
		string product_name = query->product_name();

		
		return Status::OK;
	}
};

int main(int argc, char** argv) {
	std::cout << "I 'm not ready yet!" << std::endl;
	return EXIT_SUCCESS;
}

