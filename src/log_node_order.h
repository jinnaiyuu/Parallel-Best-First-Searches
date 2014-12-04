#include <vector>
#include <atomic>

class LogNodeOrder {
public:
  LogNodeOrder() : globalOrder(0){};
  void addStateInfo(uint64_t state, int fvalue = -1, int openlistSize = -1, 
		    int thread_id = -1) {
    StateInfo* newLog = new StateInfo(globalOrder.fetch_add(1), state, 
				      fvalue, openlistSize, thread_id);
    info.push_back(*newLog);
  }

  void addStateInfo(const vector<unsigned int> *state, int fvalue = -1, int openlistSize = -1, int thread_id = -1) {
    uint64_t packedState = pack(state);
    StateInfo* newLog = new StateInfo(globalOrder.fetch_add(1), packedState, 
				      fvalue, openlistSize, thread_id);
    info.push_back(*newLog);
  }


  // This dump is not so well organized for compatibility.
  void dumpAll() {
    for (int i = 0; i < info.size(); ++i) {
      StateInfo st = info[i];
      printf("%d %d %016lx %d %d\n", st.thread_id, st.globalOrder,
	     st.state, st.fvalue, st.openlistSize);
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
	      int openlistSize_, int thread_id_) :
      globalOrder(globalOrder_), state(state_), fvalue(fvalue_),
      openlistSize(openlistSize_), thread_id(thread_id_) {}
  };
  std::atomic<int> globalOrder;
  std::vector<StateInfo> info;

};
