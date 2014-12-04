#if !defined(_LOG_NODE_ORDER_H_)
#define _LOG_NODE_ORDER_H_

#include <vector>
#include <atomic>
#include <stdio.h>

class LogNodeOrder {
public:
  LogNodeOrder() {};
  void addStateInfo(int globalOrder, uint64_t state, int fvalue = -1, int openlistSize = -1) {
    
    StateInfo* si = new StateInfo(globalOrder, state, fvalue, openlistSize);
    info.push_back(*si);
  }

  void addStateInfo(int globalOrder, const vector<unsigned int> *state, int fvalue = -1, int openlistSize = -1) {
    StateInfo* si = new StateInfo(globalOrder, pack(state), fvalue, openlistSize);
    info.push_back(*si);
  }


  // This dump is not so well organized for compatibility.
  void dumpAll(int thread_id) {
    for (int i = 0; i < info.size(); ++i) {
      StateInfo st = info[i];
      printf("%d %d %016lx %f %d\n", thread_id, st.globalOrder,
	     st.state, st.fvalue / 10000.0, st.openlistSize);
    }
  }

  uint64_t pack(const vector<unsigned int>* tiles) {
    uint64_t word = 0; // to make g++ shut up about uninitialized usage.
    for (int i = 0; i < 16; i++) {
      word = (word << 4) | (*tiles)[i];
    }
    return word;
  }


private:
  struct StateInfo {
    int globalOrder;
    uint64_t state;
    int fvalue;
    int openlistSize;
    int thread_id;
    StateInfo(int globalOrder_, uint64_t state_, int fvalue_,
	      int openlistSize_) :
      globalOrder(globalOrder_), state(state_), fvalue(fvalue_),
      openlistSize(openlistSize_) {
      //      printf("expd %d\n", globalOrder);
    }
  };
  std::vector<StateInfo> info;
};

#endif /* !_LOG_NODE_ORDER_H_ */
