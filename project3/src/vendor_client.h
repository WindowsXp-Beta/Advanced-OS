#include <string>
#include <utility>
#include <vector>

#include "vendor.grpc.pb.h"

using namespace std;

using vendor::Vendor;

class VendorClient {
 public:
  VendorClient(vector<string> vendor_ips);
  vector<pair<double, string>> getProductBid(string produce_name);

 private:
  vector<string> vendor_ips_;
  vector<unique_ptr<Vendor::Stub>> stubs_;
};