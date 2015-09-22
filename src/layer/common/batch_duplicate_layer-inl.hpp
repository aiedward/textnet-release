#ifndef TEXTNET_LAYER_BATCH_DUPLICATE_LAYER_INL_HPP_
#define TEXTNET_LAYER_BATCH_DUPLICATE_LAYER_INL_HPP_

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

#include <mshadow/tensor.h>
#include "../layer.h"
#include "../op.h"

namespace textnet {
namespace layer {

template<typename xpu>
class BatchDuplicateLayer : public Layer<xpu>{
 public:
  BatchDuplicateLayer(LayerType type) { this->layer_type = type; }
  virtual ~BatchDuplicateLayer(void) {}
  
  virtual int BottomNodeNum() { return 1; }
  virtual int TopNodeNum() { return 1; }
  virtual int ParamNodeNum() { return 0; }
  
  virtual void Require() {
    // default value, just set the value you want
	
    // require value, set to SettingV(),
    // it will force custom to set in config
    this->defaults["dup_count"] = SettingV();
    
    Layer<xpu>::Require();
  }
  
  virtual void SetupLayer(std::map<std::string, SettingV> &setting,
                          const std::vector<Node<xpu>*> &bottom,
                          const std::vector<Node<xpu>*> &top,
                          mshadow::Random<xpu> *prnd) {
    Layer<xpu>::SetupLayer(setting, bottom, top, prnd);
    
    dup_count = setting["dup_count"].iVal();

	utils::Check(dup_count > 0, 
			      "BatchDuplicateLayer: dup_count need > 0.");

    utils::Check(bottom.size() == BottomNodeNum(),
                  "BatchDuplicateLayer:bottom size problem."); 
    utils::Check(top.size() == TopNodeNum(),
                  "BatchDuplicateLayer:top size problem.");

  }
  
  virtual void Reshape(const std::vector<Node<xpu>*> &bottom,
                       const std::vector<Node<xpu>*> &top,
                       bool show_info = false) {
    utils::Check(bottom.size() == BottomNodeNum(),
                  "BatchDuplicateLayer:bottom size problem."); 
    utils::Check(top.size() == TopNodeNum(),
                  "BatchDuplicateLayer:top size problem.");
                  
    nbatch = bottom[0]->data.size(0); 

	top_nbatch = nbatch * dup_count;
                  
    top[0]->Resize(top_nbatch, bottom[0]->data.size(1), bottom[0]->data.size(2), bottom[0]->data.size(3), top_nbatch, bottom[0]->length.size(1), true);

    if (show_info) {
        bottom[0]->PrintShape("bottom0");
        top[0]->PrintShape("top0");
    }
  }

  virtual void CheckReshape(const std::vector<Node<xpu>*> &bottom,
                            const std::vector<Node<xpu>*> &top) {
    // Check for reshape
    bool need_reshape = false;
    if (nbatch != bottom[0]->data.size(0)) {
        need_reshape = true;
    }

    // Do reshape 
    if (need_reshape) {
        this->Reshape(bottom, top);
    }
  }
  
  virtual void Forward(const std::vector<Node<xpu>*> &bottom,
                       const std::vector<Node<xpu>*> &top) {
    using namespace mshadow::expr;
    mshadow::Tensor<xpu, 2> bottom_data2 = bottom[0]->data_d2();
	mshadow::Tensor<xpu, 2> bottom_len = bottom[0]->length;
    mshadow::Tensor<xpu, 2> top_data2 = top[0]->data_d2();
	mshadow::Tensor<xpu, 2> top_len = top[0]->length;

	int bottom_ptr = 0;
	int top_ptr = 0;

	while (top_ptr < top_nbatch) {
		for (int i = 0; i < dup_count; ++i) {
			top_data2[top_ptr] = F<op::identity>(bottom_data2[bottom_ptr]);
			top_len[top_ptr] = F<op::identity>(bottom_len[bottom_ptr]);
			++top_ptr;
		}
		++bottom_ptr;
	}
  }
  
  virtual void Backprop(const std::vector<Node<xpu>*> &bottom,
                        const std::vector<Node<xpu>*> &top) {
    using namespace mshadow::expr;
    mshadow::Tensor<xpu, 2> bottom_diff2 = bottom[0]->diff_d2();
    mshadow::Tensor<xpu, 2> top_diff2 = top[0]->diff_d2();

	int bottom_ptr = 0;
	int top_ptr = 0;

	while (top_ptr < top_nbatch) {
		for (int i = 0; i < dup_count; ++i) {
			bottom_diff2[bottom_ptr] += F<op::identity>(top_diff2[top_ptr]);
			++top_ptr;
		}
		++bottom_ptr;
	}
  }
  
 protected:
  int top_nbatch;
  int nbatch;
  int dup_count;

};
}  // namespace layer
}  // namespace textnet
#endif  // LAYER_BATCH_DUPLICATE_LAYER_INL_HPP_

