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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS
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

#ifndef __openma_body_segment_h
#define __openma_body_segment_h

#include "openma/body_export.h"
#include "openma/base/node.h"
#include "openma/base/macros.h" // _OPENMA_NOEXCEPT

OPENMA_EXPORT_NODE_CAST_2(ma, body, Segment, OPENMA_BODY_EXPORT);

namespace ma
{
namespace body
{  
  class SegmentPrivate;
  
  class OPENMA_BODY_EXPORT Segment : public Node
  {
    OPENMA_DECLARE_PIMPL_ACCESSOR(Segment)
    OPENMA_DECLARE_NODEID(Segment, Node)
    
  public:
    Segment(const std::string& name, int part = 0, int side = 0, Node* parent = nullptr);
    ~Segment() _OPENMA_NOEXCEPT;
    
    Segment(const Segment& ) = delete;
    Segment(Segment&& ) _OPENMA_NOEXCEPT = delete;
    Segment& operator=(const Segment& ) = delete;
    Segment& operator=(Segment&& ) _OPENMA_NOEXCEPT = delete;
    
    int part() const _OPENMA_NOEXCEPT;
    void setPart(int value) _OPENMA_NOEXCEPT;
    
    int side() const _OPENMA_NOEXCEPT;
    void setSide(int value) _OPENMA_NOEXCEPT;
    
    Segment* clone(Node* parent = nullptr) const;
    virtual void copy(const Node* source) _OPENMA_NOEXCEPT override;
  };
};
};

#endif // __openma_body_segment_h