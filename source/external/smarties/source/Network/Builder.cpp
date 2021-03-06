//
//  smarties
//  Copyright (c) 2018 CSE-Lab, ETH Zurich, Switzerland. All rights reserved.
//  Distributed under the terms of the MIT license.
//
//  Created by Guido Novati (novatig@ethz.ch).
//

#include "Builder.h"
#include "Optimizer.h"
#include "Network.h"
#include "../Utils/SstreamUtilities.h"
#include "Layers/Layer_Base.h"
#include "Layers/Layer_Conv2D.h"
#include "Layers/Layer_LSTM.h"
#include "Layers/Layer_GRU.h"

namespace smarties
{

void Builder::addInput(const Uint size)
{
  if(size==0) die("Requested an empty input layer");
  if(bBuilt) die("Cannot build the network multiple times");
  const Uint ID = layers.size();
  layers.emplace_back(
    std::make_unique<InputLayer>(size, ID)
  );
  assert(layers[ID]->nOutputs() == size);

  // if this is not first layer, glue this layer and previous together:
  if(nInputs > 0) {
    assert(ID>0);
    const Uint twoLayersSize = layers[ID-1]->nOutputs() + size;
    layers.emplace_back(
      std::make_unique<JoinLayer>(ID+1, twoLayersSize, 2)
    );
  } else assert(ID == 0);
  nInputs += size;
}

void Builder::addLayer(const Uint layerSize,
                       const std::string funcType,
                       const bool isOutputLayer,
                       const std::string layerType,
                       const Uint iLink)
{
  if(bBuilt) die("Cannot build the network multiple times");

  const Uint ID = layers.size();
  if(iLink<1 || ID<iLink || layers[ID-iLink]==nullptr || nInputs==0)
    die("Missing input layer.");
  if(layerSize <= 0)  die("Requested empty layer.");

  const Uint layerInputSize = layers[ID-iLink]->nOutputs();

  if (layerType == "LSTM")
  {
    layers.emplace_back(
      std::make_unique<LSTMLayer>(ID, layerInputSize, layerSize, funcType, isOutputLayer, iLink)
    );
  }
  else if (layerType == "MGU" || layerType == "GRU")
  {
    layers.emplace_back(
      std::make_unique<MGULayer>(ID, layerInputSize, layerSize, funcType, isOutputLayer, iLink)
    );
  }
  else
  {
    const bool bRecurrent = (layerType=="RNN") || (layerType=="Recurrent");
    layers.emplace_back(
      std::make_unique<BaseLayer>(ID, layerInputSize, layerSize, funcType, bRecurrent, isOutputLayer, iLink)
    );
  }

  #if 0
    const bool bResidualLayer = layers[ID-1]->nOutputs() == layerSize
                                && not isOutputLayer;
    if(bResidualLayer)
      layers.emplace_back(std::make_unique<ResidualLayer>(ID+1, layerSize));
  #else
    // skip connection except for output layer (because we might add non linear
    // activation to bound output) and except for first layer after input
    // because input vector may be very irregularly distributed and cause nans
    const bool bResidualLayer = not isOutputLayer and not (ID == 1);
    if(bResidualLayer)
      layers.emplace_back(
        std::make_unique<ParametricResidualLayer>(ID+1, layerSize) );
  #endif

  if(isOutputLayer) nOutputs += layers.back()->nOutputs();
}

void Builder::addParamLayer(Uint size, std::string funcType, Real init_vals)
{
  addParamLayer(size, funcType, std::vector<Real>(size, init_vals) );
}

void Builder::addParamLayer(Uint size,
                            std::string funcType,
                            std::vector<Real> init_vals)
{
  const Uint ID = layers.size();
  if(bBuilt) die("Cannot build the network multiple times\n");
  if(size<=0) die("Requested an empty layer\n");
  layers.emplace_back(
    std::make_unique<ParamLayer>(ID, size, funcType, init_vals)
  );
  nOutputs += layers.back()->nOutputs();
}

void Builder::build(const bool isInputNet)
{
  if(bBuilt) die("Cannot build the network multiple times\n");
  bBuilt = true;

  nLayers = layers.size();

  std::shared_ptr<Parameters> weights = Network::allocParameters(layers);

  // Initialize weights
  for(const auto & l : layers)
    l->initialize(gen, weights.get(),
      l->bOutput && 1);

 for(const auto & l : layers) printf( "%s", l->printSpecs().c_str() );

  // Initialize network workspace to check that all is ok
  const std::unique_ptr<Activation> test = Network::allocActivation(layers);
  if(test->nInputs not_eq (int) nInputs)
    _die("Mismatch between Builder's computed inputs:%u and Activation's:%u",
         nInputs, test->nInputs);

  if(test->nOutputs not_eq (int) nOutputs) {
    _warn("Mismatch between Builder's computed outputs:%u and Activation's:%u. "
          "Overruled Builder: probable cause is that user net did not specify "
          "which layers are output. If multiple output layers expect trouble\n",
          nOutputs, test->nOutputs);
    nOutputs = test->nOutputs;
  }

  gradients = allocManyParams(weights);

  net = std::make_shared<Network>(nInputs, nOutputs, layers, weights);
  // ownership of layers passed onto network, builder should have an empty vec:
  assert(layers.size() == 0);

  opt = std::make_shared<AdamOptimizer>(weights, gradients);
}

template
< int InX, int InY, int InC, //input image: x:width, y:height, c:color channels
  int KnX, int KnY, int KnC, //filter:      x:width, y:height, c:color channels
  int Sx, int Sy, int Px, int Py, // stride and padding x/y
  int OpX, int OpY //output img: x:width, y:height, same color channels as KnC
>
inline bool ifMatchAddConv2D(const bool bOutput, const Uint iLink,
      const Conv2D_Descriptor & DESCR,
      std::vector<std::unique_ptr<Layer>> & layers)
{
  bool sameInp = DESCR.inpFeatures==InC && DESCR.inpY==InX && DESCR.inpX==InY;
  bool sameOut = DESCR.outFeatures==KnC && DESCR.outY==OpY && DESCR.outX==OpX;
  bool sameFilter  = DESCR.filterx==KnX && DESCR.filtery==KnY;
  bool sameStride  = DESCR.stridex== Sx && DESCR.stridey== Sy;
  bool samePadding = DESCR.paddinx== Px && DESCR.paddiny== Py;
  if( KnC*OpX*OpY == 0 ) die("Requested empty layer.");
  if(sameInp && sameOut && sameFilter && sameStride && samePadding)
  {
    #ifndef USE_OMPSIMD_BLAS
      layers.emplace_back(std::make_unique<
        Mat2ImLayer<InX,InY,InC, KnX,KnY,KnC, Sx,Sy, Px,Py, OpX,OpY> >
          (layers.size(), false, iLink) );
    #endif
    layers.emplace_back(std::make_unique<
      Conv2DLayer<SoftSign, InX,InY,InC, KnX,KnY,KnC, Sx,Sy, Px,Py, OpX,OpY> >
        (layers.size(), bOutput, 1) );
    return true;
  }
  return false;
}

void Builder::addConv2d(const Conv2D_Descriptor& descr, bool bOut, Uint iLink)
{
  if(bBuilt) die("Cannot build the network multiple times");
  const Uint ID = layers.size();
  if(iLink<1 || ID<iLink || layers[ID-iLink]==nullptr || nInputs==0)
    die("Missing input layer.");

  const Uint inpSize = descr.inpFeatures * descr.inpY * descr.inpX;
  if( layers[ID-iLink]->nOutputs() not_eq inpSize )
    _die("Mismatch between input size (%d) and previous layer size (%d).",
      inpSize, layers.back()->nOutputs() );

  // I defined here the conv layers used in the Atari paper. To add new ones add
  // an if-pattern matching the other ones and refer to the `ifMatchAddConv2D`
  // function above to interpret the arguments. Useful rule of thumb to remember
  // is: outSize should be : (InSize - FilterSize + 2*Padding)/Stride + 1
  bool foundStaticDefinition = false;
  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D<84,84, 4, 8,8,32, 4,4,0,0, 20,20>(bOut,iLink,descr,layers);
  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D<20,20,32, 4,4,64, 2,2,0,0,  9, 9>(bOut,iLink,descr,layers);
  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D< 9, 9,64, 3,3,64, 1,1,0,0,  7, 7>(bOut,iLink,descr,layers);

  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D<84,84, 4, 8,8, 8, 4,4,0,0, 20,20>(bOut,iLink,descr,layers);
  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D<20,20, 8, 6,6,16, 2,2,0,0,  8, 8>(bOut,iLink,descr,layers);
  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D< 8, 8,16, 4,4,32, 1,1,0,0,  5, 5>(bOut,iLink,descr,layers);
  foundStaticDefinition = foundStaticDefinition or
    ifMatchAddConv2D< 5, 5,32, 3,3,64, 1,1,0,0,  3, 3>(bOut,iLink,descr,layers);

  if(not foundStaticDefinition)
    die("Detected undeclared conv2d description. This will be frustrating... "
        "In order to remove dependencies, keep the code low latency, and high "
        "performance, conv2d are templated. Whatever conv2d op you want must "
        "be specified in the Builder.cpp file. You'll see, it's easy.");

  assert(layers.size()>ID && layers.back());
  if(bOut) nOutputs += layers.back()->nOutputs();
}

} // end namespace smarties
