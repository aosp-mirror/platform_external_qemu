#include <unordered_map>
#include <vector>
#include <set>
#include "android/base/synchronization/Lock.h"

namespace android {
namespace base {


class Receiver {
 public:
  virtual void send(std::string msg);
};


class Pubsub {
  using Topic = std::string;
public:
  void connect(Receiver* r);
  void disconnect(Receiver* r);
  void publish(Topic t, std::string msg);
  void subscribe(Receiver* r, Topic t);
  void unsubscribe(Receiver* r, Topic t);

private:

  std::unordered_map<Topic, std::set<Receiver*>> mTopics;
  std::unordered_map<Receiver*, std::set<Topic>> mSubscribed;
  std::set<Receiver*> mConnected; 
  Lock mLock;
};
}  // namespace base
}  // namespace android
