/* 
 * Open Source Movement Analysis Library
 * Copyright (C) 2016, Moveck Solution Inc., all rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name(s) of the copyright holders nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "openma/body/model.h"
#include "openma/body/model_p.h"
#include "openma/body/segment.h"
#include "openma/body/joint.h"
#include "openma/body/chain.h"

// -------------------------------------------------------------------------- //
//                                 PRIVATE API                                //
// -------------------------------------------------------------------------- //

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace ma
{
namespace body
{
  ModelPrivate::ModelPrivate(Model* pint, const std::string& name)
  : NodePrivate(pint,name)
#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 1900))
  , Gravity{0.,0.,0.}
#endif
  {
#if defined(_MSC_VER) && (_MSC_VER < 1900)
    this->Gravity[0] = 0.;
    this->Gravity[1] = 0.;
    this->Gravity[2] = 0.;
#endif
  };
  
  ModelPrivate::~ModelPrivate() = default;
};
};

#endif

// -------------------------------------------------------------------------- //
//                                 PUBLIC API                                 //
// -------------------------------------------------------------------------- //

OPENMA_INSTANCE_STATIC_TYPEID(ma::body::Model);

namespace ma
{
namespace body
{
  /**
   * @class Model openma/body/model.h
   * @brief Tree-like multibody class.
   * A Model class is a helper to manage and store a multibody system composed of Segment and Joint objects.
   * @todo Detail more the use of the Model class with exemple on the creation of segments and joints.
   * @ingroup openma_body
   */
  
  /**
   * Constructor
   */
  Model::Model(const std::string& name, Node* parent)
  : Node(*new ModelPrivate(this,name), parent)
  {};
  
  /**
   * Destructor (default)
   */
  Model::~Model() _OPENMA_NOEXCEPT = default;
  
  /**
   * Retuns an array of 3 elements representing the gravity along the axes of the Inertial Coordinate System (ICS, also known as the laboratory coordinate system)
   * By default, the gravity is not set and correponds to a vector of zeros.
   */
  const double* Model::gravity() const _OPENMA_NOEXCEPT
  {
    auto optr = this->pimpl();
    return optr->Gravity;
  };
  
  /**
   * Set the value for the gravity along the axes of the ICS.
   * By default, the gravity is not set and correponds to a vector of zeros.
   */
  void Model::setGravity(const double value[3]) _OPENMA_NOEXCEPT
  {
    auto optr = this->pimpl();
    if ((optr->Gravity[0] == value[0])
      && (optr->Gravity[1] == value[1])
        && (optr->Gravity[2] == value[2]))
          return;
    std::copy_n(value,3,optr->Gravity);
    this->modified();
  };
  
  /**
   * Returns the subnode "Segments".
   * If the subnode does not exist, this one is created.
   */
  Node* Model::segments()
  {
    auto pt = this->findChild("Segments",{},false);
    if (pt == nullptr)
      pt = new Node("Segments",this);
    return pt;
  };
  
  /**
   * Returns the @a idx child of the subnode "Segments" and cast it as a Segemnt object.
   * If @a idx is out of range or if the extracted node is not a Segemnt object, the method returns nullptr.
   */
  Segment* Model::segment(unsigned idx) _OPENMA_NOEXCEPT
  {
    return this->segments()->child<Segment*>(idx);
  };

  /**
   * Returns the subnode "Joints".
   * If the subnode does not exist, this one is created.
   */
  Node* Model::joints()
  {
    auto pt = this->findChild("Joints",{},false);
    if (pt == nullptr)
      pt = new Node("Joints",this);
    return pt;
  };
  
  /**
   * Returns the @a idx child of the subnode "Joints" and cast it as a Joint object.
   * If @a idx is out of range or if the extracted node is not a Joint object, the method returns nullptr.
   */
  Joint* Model::joint(unsigned idx) _OPENMA_NOEXCEPT
  {
    return this->joints()->child<Joint*>(idx);
  };
  
  /**
   * Returns the subnode "Chains".
   * If the subnode does not exist, this one is created.
   */
  Node* Model::chains()
  {
    auto pt = this->findChild("Chains",{},false);
    if (pt == nullptr)
      pt = new Node("Chains",this);
    return pt;
  };
  
  /**
   * Returns the @a idx child of the subnode "Chains" and cast it as a Chain object.
   * If @a idx is out of range or if the extracted node is not a Chain object, the method returns nullptr.
   */
  Chain* Model::chain(unsigned idx) _OPENMA_NOEXCEPT
  {
    return this->joints()->child<Chain*>(idx);
  };
  
  
  /**
   * Create a deep copy of the object and return it as another object.
   */
  Model* Model::clone(Node* parent) const
  {
    auto dest = new Model(this->name());
    dest->copy(this);
    dest->addParent(parent);
    return dest;
  };
  
  /**
   * Do a deep copy of the the given @a source. The previous content is replaced.
   */
  void Model::copy(const Node* source) _OPENMA_NOEXCEPT
  {
    auto src = node_cast<const Model*>(source);
    if (src == nullptr)
      return;
    this->Node::copy(src);
  };
};
};