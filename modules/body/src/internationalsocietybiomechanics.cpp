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

#include "openma/body/internationalsocietybiomechanics.h"
#include "openma/body/internationalsocietybiomechanics_p.h"
#include "openma/body/anchor.h"
#include "openma/body/chain.h"
#include "openma/body/dumasmcconvilleyoungtable.h"
#include "openma/body/dynamicdescriptor.h"
#include "openma/body/eulerdescriptor.h"
#include "openma/body/enums.h"
#include "openma/body/eulerdescriptor.h"
#include "openma/body/inversedynamicsmatrix.h"
#include "openma/body/joint.h"
#include "openma/body/landmarkstranslator.h"
#include "openma/body/model.h"
#include "openma/body/point.h"
#include "openma/body/referenceframe.h"
#include "openma/body/segment.h"
#include "openma/body/utils.h"
#include "openma/base/enums.h"
#include "openma/base/subject.h"
#include "openma/base/trial.h"
#include "openma/math.h"

#include <iostream>

// -------------------------------------------------------------------------- //
//                                 PRIVATE API                                //
// -------------------------------------------------------------------------- //

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace ma
{
namespace body
{
  InternationalSocietyBiomechanicsPrivate::InternationalSocietyBiomechanicsPrivate(InternationalSocietyBiomechanics* pint, const std::string& name, int region, int side)
  : SkeletonHelperPrivate(pint,name,region,side),
    // All the options are null by default
    Sex_(Sex::Unknown)
  {};
  
  InternationalSocietyBiomechanicsPrivate::~InternationalSocietyBiomechanicsPrivate() _OPENMA_NOEXCEPT = default;
  
  
  bool InternationalSocietyBiomechanicsPrivate::calibrateUpperLimb(int side, const math::Pose* torso, TaggedPositions* landmarks)
  {
    auto pptr = this->pint();
    std::string prefix;
    double s = 0.0, segLength = 0.0;
    if (side == Side::Left)
    {
      prefix = "L.";
      s = -1.0;
      
    }
    else if (side == Side::Right)
    {
      prefix = "R.";
      s = 1.0;
    }
    // -----------------------------------------
    // Arm
    // -----------------------------------------
    // Required landmarks: *.AC, *.GH, *.MHE, *.LHE
    const auto& AC = (*landmarks)[prefix+"AC"];
    const auto& GH = (*landmarks)[prefix+"GH"];
    const auto& MHE = (*landmarks)[prefix+"MHE"];
    const auto& LHE = (*landmarks)[prefix+"LHE"];
    if (!AC.isValid() || !GH.isValid() || !MHE.isValid() || !LHE.isValid())
    {
      error("InternationalSocietyBiomechanics - Missing landmarks to define the arm. Calibration aborted.");
      return false;
    }
    math::Position SJC(1); ; SJC.residuals().setZero();
    double r = 0., percent = 0.;
    if (this->Sex_ == Sex::Female)
    {
      r = -5.0 * M_PI / 180.0; 
      percent = 0.53;
    }
    else
    {
      r = -11.0 * M_PI / 180.0;
      percent = 0.43;
    }
    
    //   - SJC
    Point* SJCH = nullptr;
    if ((SJCH = pptr->findChild<Point*>(prefix+"SJC",{},false)) == nullptr)
    {
      double depth = pptr->property("torso.depth");
      math::Pose rotInv(1); rotInv.values().setZero(); rotInv.residuals().setZero();
      rotInv.values() << cos(r), sin(r), 0., -sin(r), cos(r), 0., 0., 0., 1., 0., 0., 0.;
      math::Position lSJC(1); lSJC.residuals().setZero();
      lSJC.values() << percent * depth, 0.0, 0.0;
      SJC = torso->transform(rotInv.transform(lSJC));
    }
    else
      std::copy_n(SJCH->data(), 3, SJC.values().data());
    math::Position EJC = (LHE + MHE) / 2.0;
    math::Vector v = (GH - EJC).normalized();
    math::Vector u = v.cross(s * (LHE - MHE)).normalized();
    math::Pose tcs_pose(u,v,u.cross(v),GH);
    v = (SJC - EJC).normalized();
    u = v.cross(s * (LHE - MHE)).normalized();
    math::Pose scs_relpose = tcs_pose.inverse().transform(math::Pose(u,v,u.cross(v),SJC));
    auto scs_node = pptr->findChild<ReferenceFrame*>(prefix+"Arm.SCS",{},false);
    if (scs_node == nullptr)
      new ReferenceFrame(prefix+"Arm.SCS", scs_relpose.values().data(), pptr);
    else
      scs_node->setData(scs_relpose.values().data());
    segLength = (SJC - EJC).norm();
    pptr->setProperty(prefix+"Arm.length", segLength);
    // -----------------------------------------
    // Forearm
    // -----------------------------------------
    // Required landmarks: *.US, *.RS
    const auto& US = (*landmarks)[prefix+"US"];
    const auto& RS = (*landmarks)[prefix+"RS"];
    if (!US.isValid() || !RS.isValid())
    {
      error("InternationalSocietyBiomechanics - Missing landmarks to define the forearm. Calibration aborted.");
      return false;
    }
    math::Position WJC = (US + RS) / 2.0;
    segLength = (EJC - WJC).norm();
    pptr->setProperty(prefix+"Forearm.length", segLength);
    // -----------------------------------------
    // Hand
    // -----------------------------------------
    // Required landmarks: *.MH2, *.MH5
    const auto& MH2 = (*landmarks)[prefix+"MH2"];
    const auto& MH5 = (*landmarks)[prefix+"MH5"];
    if (!MH2.isValid() || !MH5.isValid())
    {
      error("InternationalSocietyBiomechanics - Missing landmarks to define the hand. Calibration aborted.");
      return false;
    }
    math::Position MH = (MH2 + MH5) / 2.0;
    segLength = (WJC - MH).norm();
    pptr->setProperty(prefix+"Hand.length", segLength);
    return true;
  };
  
  bool InternationalSocietyBiomechanicsPrivate::calibrateLowerLimb(int side, const math::Position* HJC, TaggedPositions* landmarks)
  {
    auto pptr = this->pint();
    math::Vector u, v, w;
    math::Pose tcs_pose, scs_relpose;
    ReferenceFrame* scs_node;
    std::string prefix;
    double s = 0.0, segLength = 0.0;
    if (side == Side::Left)
    {
      prefix = "L.";
      s = -1.0;
      
    }
    else if (side == Side::Right)
    {
      prefix = "R.";
      s = 1.0;
    }
    // -----------------------------------------
    // Thigh
    // -----------------------------------------
    // Required landmarks: *.GT, *.LFE and *.MFE
    const auto& GT = (*landmarks)[prefix+"GT"];
    const auto& LFE = (*landmarks)[prefix+"LFE"];
    const auto& MFE = (*landmarks)[prefix+"MFE"];
    if (!GT.isValid() || !LFE.isValid() || !MFE.isValid())
    {
      error("InternationalSocietyBiomechanics - Missing landmarks to define the thigh. Calibration aborted.");
      return false;
    }
    //  - Technical frame
    v = (GT - LFE).normalized();
    u = v.cross(s * (LFE - MFE)).normalized();
    tcs_pose = math::Pose(u,v,u.cross(v),LFE);
    //  - Relative segmental frame
    math::Position KJC = (LFE + MFE) / 2.0;
    v = (*HJC - KJC).normalized();
    u = v.cross(s * (LFE - MFE)).normalized();
    scs_relpose = tcs_pose.inverse().transform(math::Pose(u,v,u.cross(v),*HJC));
    scs_node = pptr->findChild<ReferenceFrame*>(prefix+"Thigh.SCS",{},false);
    if (scs_node == nullptr)
      new ReferenceFrame(prefix+"Thigh.SCS", scs_relpose.values().data(), pptr);
    else
      scs_node->setData(scs_relpose.values().data());
    segLength = (*HJC - KJC).norm();
    pptr->setProperty(prefix+"Thigh.length", segLength);
    // -----------------------------------------
    // Shank
    // -----------------------------------------
    // Required landmarks: *.LFE, *.MFE, *.FH, *.LTM, *.MTM
    const auto& LTM = (*landmarks)[prefix+"LTM"];
    const auto& MTM = (*landmarks)[prefix+"MTM"];
    if (!LTM.isValid() || !MTM.isValid())
    {
      error("InternationalSocietyBiomechanics - Missing landmarks to define the shank. Calibration aborted.");
      return false;
    }
    math::Position AJC = (LTM + MTM) / 2.0;
    segLength = (KJC - AJC).norm();
    pptr->setProperty(prefix+"Shank.length", segLength);
    // -----------------------------------------
    // Foot
    // -----------------------------------------
    // Required landmarks: *.LTM, *.MTM, *.MTH1, *.MTH5, *.HEE
    const auto& MTH1 = (*landmarks)[prefix+"MTH1"];
    const auto& MTH5 = (*landmarks)[prefix+"MTH5"];
    const auto& HEE = (*landmarks)[prefix+"HEE ="];
    if (!MTH1.isValid() || !MTH5.isValid() || !HEE.isValid())
    {
      error("InternationalSocietyBiomechanics - Missing landmarks to define the foot. Calibration aborted.");
      return false;
    }
    segLength = (((MTH1 + MTH5) / 2.0) - HEE).norm();
    pptr->setProperty(prefix+"Foot.length", segLength);
    return true;
  };
  
  bool InternationalSocietyBiomechanicsPrivate::reconstructUpperLimb(Model* model, Trial* trial, int side, TaggedMappedPositions* landmarks, double sampleRate, double startTime) const _OPENMA_NOEXCEPT
  {
    return true;
  };
  
  bool InternationalSocietyBiomechanicsPrivate::reconstructLowerLimb(Model* model, Trial* trial, int side, TaggedMappedPositions* landmarks, double sampleRate, double startTime) const _OPENMA_NOEXCEPT
  {
    return true;
  };
};
};

#endif

// -------------------------------------------------------------------------- //
//                                 PUBLIC API                                 //
// -------------------------------------------------------------------------- //

OPENMA_INSTANCE_STATIC_TYPEID(body::InternationalSocietyBiomechanics);

namespace ma
{
namespace body
{
  /**
   * @class InternationalSocietyBiomechanics openma/body/internationalsocietybiomechanics.h
   * @brief Helper class to create and reconstruct a model based on the International Society of Biomechanics (ISB) recommendations.
   *
   * @todo Add a detailed description on possible configurations, segments, joints, and coordinate systems definition. 
   * @todo Add the required and optional parameters used by this helper.
   * @todo Explain how to set custom hip joint centre
   * @todo Explain how to set custom lumbar joint centre
   * @todo Explain how to set custom shoulder joint centre
   *
   * @ingroup openma_body
  */
  
#ifdef DOXYGEN_SHOULD_TAKE_THIS
  /** * @brief Fake structure to create node's properties */
  struct InternationalSocietyBiomechanics::__Doxygen_Properties
  {
  /**
   * [Required] This property holds the subject's sex used in the computation of default hip joint centre and shoulder joint centre. By default, this property contains the value Sex::Unknown.
   * @sa sex() setSex()
   */
  Sex sex;
  };
#endif
 
  /**
   * Constructor. The name is set to "InternationalSocietyBiomechanics".
   */
  InternationalSocietyBiomechanics::InternationalSocietyBiomechanics(int region, int side, Node* parent)
  : SkeletonHelper(*new InternationalSocietyBiomechanicsPrivate(this, "InternationalSocietyBiomechanics", region, side), parent)
  {};
  
  /**
   * Create segments and joints according to the region and side given to the constructor.
   */
  bool InternationalSocietyBiomechanics::setupModel(Model* model) const
  {
    if (model == nullptr)
    {
      error("Null model passed. Setup aborted.");
      return false;
    }
    auto optr = this->pimpl();
    if (optr->Side == Side::Center)
    {
      error("Impossible to have a 'centre' side for the InternationalSocietyBiomechanics helper. Only the enumerators Side::Right, Side::Left, and Side::Both are available for this helper. Setup aborted.");
      return false;
    }
    if (optr->Sex_ == Sex::Unknown)
    {
      error("Unknown sex. Setup aborted.");
      return false;
    }
    Node* segments = model->segments();
    Node* joints = model->joints();
    Node* chains = model->chains();
    Segment *torso = nullptr, *pelvis = nullptr,
            *progression = new Segment("Progression", Part::User, Side::Center, segments);
    Joint* jnt;
    if (optr->Region & Region::Upper)
    {
      model->setName(this->name() + "_UpperLimb");
      Segment* head = new Segment("Head", Part::Head, Side::Center, segments);
      torso = new Segment("Torso", Part::Torso, Side::Center, segments);
      if (optr->Side & Side::Left)
      {
        Segment* leftArm = new Segment("L.Arm", Part::Arm, Side::Left, segments);
        Segment* leftForearm = new Segment("L.Forearm", Part::Forearm, Side::Left, segments);
        Segment* leftHand = new Segment("L.Hand", Part::Hand, Side::Left, segments);
        std::vector<Joint*> leftUpperLimbJoints(3);
        jnt = new Joint("L.Shoulder", torso, leftArm, joints);
        leftUpperLimbJoints[0] = jnt;
        new EulerDescriptor("L.Shoulder.Angle", EulerDescriptor::ZXY, jnt);
        jnt = new Joint("L.Elbow", leftArm, leftForearm, joints);
        leftUpperLimbJoints[1] = jnt;
        new EulerDescriptor("L.Elbow.Angle", EulerDescriptor::ZXY, jnt);
        jnt = new Joint("L.Wrist", leftForearm, leftHand, joints);
        leftUpperLimbJoints[2] = jnt;
        new EulerDescriptor("L.Wrist.Angle", EulerDescriptor::ZXY, jnt);
        // new Chain("L.UpperLimb", leftUpperLimbJoints, chains);
      }
      if (optr->Side & Side::Right)
      {
        Segment* rightArm = new Segment("R.Arm", Part::Arm, Side::Right, segments);
        Segment* rightForearm = new Segment("R.Forearm", Part::Forearm, Side::Right, segments);
        Segment* rightHand = new Segment("R.Hand", Part::Hand, Side::Right, segments);
        std::vector<Joint*> rightUpperLimbJoints(3);
        jnt = new Joint("R.Shoulder", torso, rightArm, joints);
        rightUpperLimbJoints[0] = jnt;
        new EulerDescriptor("R.Shoulder.Angle", EulerDescriptor::ZXY, jnt);
        jnt = new Joint("R.Elbow", rightArm, rightForearm, joints);
        rightUpperLimbJoints[1] = jnt;
        new EulerDescriptor("R.Elbow.Angle", EulerDescriptor::ZXY, jnt);
        jnt = new Joint("R.Wrist", rightForearm, rightHand, joints);
        rightUpperLimbJoints[2] = jnt;
        new EulerDescriptor("R.Wrist.Angle", EulerDescriptor::ZXY, jnt);
        // new Chain("R.UpperLimb", rightUpperLimbJoints, chains);
      }
    }
    if (optr->Region & Region::Lower)
    {
      model->setName(this->name() + "_LowerLimb");
      pelvis = new Segment("Pelvis", Part::Pelvis, Side::Center, segments);
      if (optr->Side & Side::Left)
      {
        Segment* leftThigh = new Segment("L.Thigh", Part::Thigh, Side::Left, segments);
        Segment* leftShank = new Segment("L.Shank", Part::Shank, Side::Left, segments);
        Segment* leftFoot = new Segment("L.Foot", Part::Foot, Side::Left, segments);
        std::vector<Joint*> leftLowerLimbJoints(3);
        jnt = new Joint("L.Hip", pelvis, Anchor::point("L.HJC"), leftThigh, joints);
        leftLowerLimbJoints[0] = jnt;
        new EulerDescriptor("L.Hip.Angle", EulerDescriptor::ZXY, jnt);
        new DynamicDescriptor(jnt);
        jnt = new Joint("L.Knee", leftThigh, leftShank, joints);
        leftLowerLimbJoints[1] = jnt;
        new EulerDescriptor("L.Knee.Angle", EulerDescriptor::ZXY, jnt);
        new DynamicDescriptor(jnt);
        jnt = new Joint("L.Ankle", leftShank, leftFoot, joints);
        leftLowerLimbJoints[2] = jnt;
        new EulerDescriptor("L.Ankle.Angle", EulerDescriptor::ZXY, jnt);
        new DynamicDescriptor(jnt);
        new Chain("L.LowerLimb", leftLowerLimbJoints, chains);
      }
      if (optr->Side & Side::Right)
      {
        Segment* rightThigh = new Segment("R.Thigh", Part::Thigh, Side::Right, segments);
        Segment* rightShank = new Segment("R.Shank", Part::Shank, Side::Right, segments);
        Segment* rightFoot = new Segment("R.Foot", Part::Foot, Side::Right,segments);
        std::vector<Joint*> rightLowerLimbJoints(3);
        jnt = new Joint("R.Hip", pelvis, Anchor::point("R.HJC"), rightThigh, joints);
        rightLowerLimbJoints[0] = jnt;
        new EulerDescriptor("R.Hip.Angle", EulerDescriptor::ZXY, jnt);
        new DynamicDescriptor(jnt);
        jnt = new Joint("R.Knee", rightThigh, rightShank, joints);
        rightLowerLimbJoints[1] = jnt;
        new EulerDescriptor("R.Knee.Angle", EulerDescriptor::ZXY, jnt);
        new DynamicDescriptor(jnt);
        jnt = new Joint("R.Ankle", rightShank, rightFoot, Anchor::origin(rightShank), joints);
        rightLowerLimbJoints[2] = jnt;
        new EulerDescriptor("R.Ankle.Angle", EulerDescriptor::ZXY, jnt);
        new DynamicDescriptor(jnt);
        new Chain("R.LowerLimb", rightLowerLimbJoints, chains);
      }
    }
    if ((optr->Region & Region::Full) == Region::Full)
    {
      model->setName(this->name() + "_FullBody");
      jnt = new Joint("Spine", torso, pelvis, joints);
      jnt->setDescription("Torso relative to pelvis");
    }
    return true;
  };
 
  /**
   * Calibrate this helper based on the first Trial object found in @a trials. If @a subject is not a null pointer, its dynamic properties are copied to this object.
   * @todo Explain how to set custom hip joint centre
   */
  bool InternationalSocietyBiomechanics::calibrate(Node* trials, Subject* subject)
  {
    if (trials == nullptr)
    {
      error("InternationalSocietyBiomechanics - Null trials passed. Calibration aborted.");
      return false;
    }
    auto trial = trials->findChild<Trial*>();
    if (trial == nullptr)
    {
       error("InternationalSocietyBiomechanics - No trial found. Calibration aborted.");
      return false;
    }
    bool ok = false;
    auto lmks = extract_landmark_positions(this, trial, nullptr, nullptr, &ok);
    if (!ok)
    {
      error("InternationalSocietyBiomechanics - The sampling information is not consistent between required landmarks (sampling rates or start times are not the same). Calibration aborted.");
      return false;
    }
    
    auto optr = this->pimpl();
    // Copy the content of subject into the helper
    if (subject != nullptr)
    {
      const auto& props = subject->dynamicProperties();
      for (const auto& prop : props)
        this->setProperty(prop.first, prop.second);
    }
    auto sexProp = this->property("sex");
    if (!sexProp.isValid())
    {
      error("InternationalSocietyBiomechanics - The sex of the subject is required to set correctly the model. Calibration aborted.");
      return false;
    }
    auto sex = sexProp.cast<Sex>();
    if ((sex != Sex::Male) && (sex != Sex::Female))
    {
      error("InternationalSocietyBiomechanics - Unrecognized subject's sex. Calibration aborted.");
      return false;
    }
    
    // Average the data
    std::unordered_map<std::string,math::Vector> landmarks;
    for (const auto& pos : lmks)
      landmarks.emplace(pos.first, pos.second.mean());
    
    // Calibrate the helper
    math::Position LJC(1); // Joint center used by the lower and upper parts
    
    // --------------------------------------------------
    // LOWER LIMB
    // --------------------------------------------------
    if ((optr->Region & Region::Lower) == Region::Lower)
    {
      using PosVal = Eigen::Matrix<double,3,1>;
      // Regression data for joint center definition (see Dumas et al., 2007)
      // WARNING: They are expressed in the global frame!
      PosVal R_ASIS_Dumas, L_ASIS_Dumas, R_HJC_Dumas, L_HJC_Dumas, LJC_Dumas, SC_Dumas;
      if (sex == Sex::Female)
      {
        R_ASIS_Dumas << 87.0,-13.0,119.0; // mm
        L_ASIS_Dumas << 87.0,-13.0,-119.0; // mm
        R_HJC_Dumas << 54.0,-93.0,88.0; // mm
        L_HJC_Dumas << 54.0,-93.0,-88.0; // mm
        LJC_Dumas << 0.0,0.0,0.0; // mm
        SC_Dumas << -108.0,-13.0,0.0; // mm
      }
      else
      {
        R_ASIS_Dumas << 78.0,7.0,112.0; // mm
        L_ASIS_Dumas << 78.0,7.0,-112.0; // mm
        R_HJC_Dumas << 56.0,-75.0,81.0; // mm
        L_HJC_Dumas << 56.0,-75.0,-81.0; // mm
        LJC_Dumas << 0.0,0.0,0.0; // mm
        SC_Dumas << -102.0,7.0,0.0; // mm
      }
      const double asis_length_Dumas = (R_ASIS_Dumas - L_ASIS_Dumas).norm();
      const PosVal r_R_HJC = (R_HJC_Dumas - SC_Dumas) / asis_length_Dumas;
      const PosVal r_L_HJC = (L_HJC_Dumas - SC_Dumas) / asis_length_Dumas;
      const PosVal r_LJC = (LJC_Dumas - SC_Dumas) / asis_length_Dumas;
      
      // -----------------------------------------
      // Pelvis
      // -----------------------------------------
      // Required landmarks: *.ASIS, SC or *.PSIS
      const auto& L_ASIS = landmarks["L.ASIS"];
      const auto& R_ASIS = landmarks["R.ASIS"];
      const auto& L_PSIS = landmarks["L.PSIS"];
      const auto& R_PSIS = landmarks["R.PSIS"];
      math::Position SC = landmarks["SC"];
      if (!L_ASIS.isValid() || !R_ASIS.isValid())
      {
        error("InternationalSocietyBiomechanics - Missing landmarks (ASISs) to define the pelvis. Calibration aborted.");
        return false;
      }
      
      if (!SC.isValid() && (!L_PSIS.isValid() || !R_PSIS.isValid()))
      {
        error("InternationalSocietyBiomechanics - Missing landmarks (SC or PSISs) to define the pelvis. Calibration aborted.");
        return false;
      }
      if (!SC.isValid())
        SC = (L_PSIS + R_PSIS) / 2.0;
      const math::Vector w = (R_ASIS - L_ASIS).normalized();
      const math::Vector v = (w.cross((R_ASIS + L_ASIS) / 2.0 - SC)).normalized();
      const math::Pose pelvis = math::Pose(v.cross(w), v, w, SC);
      // Compute HJCs and LJC from the article of Dumas et al., 2007.
      const double asis_length = (R_ASIS - L_ASIS).norm();
      math::Position L_HJC(1); L_HJC.residuals().setZero();
      math::Position R_HJC(1); L_HJC.residuals().setZero();
      LJC.residuals().setZero();
      //   - lumbar joint centre (LJC)
      Point* LJCH = nullptr;
      if ((LJCH = this->findChild<Point*>("LJC",{},false)) == nullptr)
      {
        LJC = SC + r_LJC.coeff(0) * asis_length * pelvis.u()
                 + r_LJC.coeff(1) * asis_length * pelvis.v()
                 + r_LJC.coeff(2) * asis_length * pelvis.w();
      }
      else
        std::copy_n(LJCH->data(), 3, LJC.values().data());
      //   - Left HJC
      Point* leftSJCH = nullptr;
      if ((leftSJCH = this->findChild<Point*>("L.HJC",{},false)) == nullptr)
      {
        L_HJC = SC + r_L_HJC.coeff(0) * asis_length * pelvis.u()
                   + r_L_HJC.coeff(1) * asis_length * pelvis.v()
                   + r_L_HJC.coeff(2) * asis_length * pelvis.w();
      }
      else
        std::copy_n(leftSJCH->data(), 3, L_HJC.values().data());
      //   - Right HJC
      Point* rightSJCH = nullptr;
      if ((rightSJCH = this->findChild<Point*>("R.HJC",{},false)) == nullptr)
      {
        R_HJC = SC + r_R_HJC.coeff(0) * asis_length * pelvis.u()
                   + r_R_HJC.coeff(1) * asis_length * pelvis.v()
                   + r_R_HJC.coeff(2) * asis_length * pelvis.w();
      }
      else
        std::copy_n(rightSJCH->data(), 3, R_HJC.values().data());
      // Set the segment length
      this->setProperty("Pelvis.length", static_cast<double>((LJC - (L_HJC + R_HJC)).norm()));
      // -----------------------------------------
      // Other lower limbs
      // -----------------------------------------
      if ((optr->Side & Side::Left) == Side::Left)
      {
        if (!optr->calibrateLowerLimb(Side::Left, &L_HJC, &landmarks))
          return false;
      }
      if ((optr->Side & Side::Right) == Side::Right)
      {
        if (!optr->calibrateLowerLimb(Side::Right, &R_HJC, &landmarks))
          return false;
       }
    }
    
    // --------------------------------------------------
    // UPPER LIMB
    // --------------------------------------------------
    if ((optr->Region & Region::Upper) == Region::Upper)
    {
      // -----------------------------------------
      // Torso
      // -----------------------------------------
      // Required landmarks: SS, C7, XP and T8
      const auto& SS = landmarks["SS"];
      const auto& C7 = landmarks["C7"];
      const auto& XP = landmarks["XP"];
      const auto& T8 = landmarks["T8"];
      if (!SS.isValid() || !C7.isValid() || !XP.isValid() || !T8.isValid())
      {
        error("InternationalSocietyBiomechanics - Missing landmarks to define the torso. Calibration aborted.");
        return false;
      }
      math::Vector u = (SS - C7).normalized();
      math::Vector v;
      math::Vector w = u.cross((SS + C7) / 2.0 - (XP + T8) / 2.0).normalized();
      const math::Pose torso = math::Pose(u, w.cross(u), w, C7);
      double depth = (SS - C7).norm();
      double r, percent;
      if (sex == Sex::Female)
      {
        r = 14.0 * M_PI / 180.0; 
        percent = 0.53;
      }
      else
      {
        r = 8.0 * M_PI / 180.0;
        percent = 0.55;
      }
      // The inverse of the matrix rot is known. Not necessary to compute it.
      math::Pose rotInv(1); rotInv.residuals().setZero();
      rotInv.values() << cos(r), sin(r), 0., -sin(r), cos(r), 0., 0., 0., 1., 0., 0., 0.;
      math::Position lCJC(1); lCJC.residuals().setZero();
      lCJC.values() << percent * depth, 0.0, 0.0;
      math::Position CJC = torso.transform(rotInv.transform(lCJC));
      v = ((C7 + SS) / 2.0 - (XP + T8) / 2.0).normalized();
      w = (SS - C7).cross(v).normalized();
      math::Pose tcs_pose(v.cross(w),v,w,C7);
      v = (CJC - (XP + T8) / 2.0).normalized();
      w = (SS - C7).cross(v).normalized();
      math::Pose scs_relpose = tcs_pose.inverse().transform(math::Pose(v.cross(w),v,w,CJC));
      auto scs_node = this->findChild<ReferenceFrame*>("Torso.SCS",{},false);
      if (scs_node == nullptr)
        new ReferenceFrame("Torso.SCS", scs_relpose.values().data(), this);
      else
        scs_node->setData(scs_relpose.values().data());
      double segLength = 0.0;
      if (!LJC.isValid())
        warning("The lumbar joint centre is not defined! Impossible to determine the length of the torso. Torso's length is set to 0.");
      else
        segLength = (CJC - LJC).norm();
      this->setProperty("Torso.length", segLength);
      this->setProperty("Torso.depth", depth);
      // -----------------------------------------
      // Other lower limbs
      // -----------------------------------------
      if ((optr->Side & Side::Left) == Side::Left)
      {
        if (!optr->calibrateUpperLimb(Side::Left, &torso, &landmarks))
          return false;
      }
      if ((optr->Side & Side::Right) == Side::Right)
      {
        if (!optr->calibrateUpperLimb(Side::Right, &torso, &landmarks))
          return false;
       }
    }
    return true;
  };
  
  /**
   * Reconstruct @a model segments' movement based on the content of the given @a trial.
   * @a todo Explain that exported segment frame corresponds to the segmental coordinate system (SCS).
   */
  bool InternationalSocietyBiomechanics::reconstructModel(Model* model, Trial* trial)
  {
    if (model == nullptr)
    {
      error("InternationalSocietyBiomechanics - Null model passed. Movement reconstruction aborted.");
      return false;
    }
    if (trial == nullptr)
    {
      error("InternationalSocietyBiomechanics - Null trial passed. Movement reconstruction aborted.");
      return false;
    }
    model->setName(model->name() + "_" + trial->name());
    double sampleRate = 0.0;
    double startTime = 0.0;
    bool ok = false;
    auto landmarks = extract_landmark_positions(this, trial, &sampleRate, &startTime, &ok);
    if (!ok)
    {
      error("InternationalSocietyBiomechanics - The sampling information are no consistent (sampling rate or start time not the same) between required landmarks. Movement reconstruction aborted for trial '%s'.", trial->name().c_str());
      return false;
    }
    auto optr = this->pimpl();
    Segment* seg = nullptr;
    // --------------------------------------------------
    // UPPER LIMB
    // --------------------------------------------------
    if (optr->Region & Region::Upper)
    {
      // -----------------------------------------
      // Torso
      // -----------------------------------------
      seg = model->segments()->findChild<Segment*>({},{{"part",Part::Torso}},false);
      // Required landmarks: SS, C7, XP and T8
      const auto& SS = landmarks["SS"];
      const auto& C7 = landmarks["C7"];
      const auto& XP = landmarks["XP"];
      const auto& T8 = landmarks["T8"];
      if (!SS.isValid() || !C7.isValid() || !XP.isValid() || !T8.isValid())
      {
        error("InternationalSocietyBiomechanics - Missing landmarks to define the torso. Movement reconstruction aborted for trial '%s'.", trial->name().c_str());
        return false;
      }
      Point* LJCH = nullptr;
      if ((LJCH = this->findChild<Point*>("LJC",{},false)) == nullptr)
      {
        error("InternationalSocietyBiomechanics - Relative lumbar joint centre not found. Did you calibrate first the helper? Movement reconstruction aborted.");
        return false;
      }
      // - Construct the relative segment coordinate system for the torso
      auto relframe = new ReferenceFrame("SCS", nullptr, LJCH->data(), seg);
      // - Construct the Torso.SCS time sequence
      const math::Vector v = ((C7 + SS) / 2.0 - (XP + T8) / 2.0).normalized();
      const math::Vector w = (SS - C7).cross(v).normalized();
      math::to_timesequence(transform_relative_frame(relframe, seg, math::Pose(v.cross(w),v,w,C7)), seg->name()+".SCS", sampleRate, startTime, TimeSequence::Pose, "", relframe);
      seg->setProperty("length", this->property(seg->name()+".length"));
      // -----------------------------------------
      // Other upper limbs (dependant of the torso)
      // -----------------------------------------
      if ((optr->Side & Side::Left) && !optr->reconstructUpperLimb(model, trial, Side::Left, &landmarks, sampleRate, startTime))
        return false;
      if ((optr->Side & Side::Right) && !optr->reconstructUpperLimb(model, trial, Side::Right, &landmarks, sampleRate, startTime))
        return false;
    }
    // --------------------------------------------------
    // LOWER LIMB
    // --------------------------------------------------
    if (optr->Region & Region::Lower)
    {
      // -----------------------------------------
      // Pelvis
      // -----------------------------------------
      seg = model->segments()->findChild<Segment*>({},{{"part",Part::Pelvis}},false);
      // Required landmarks: *.ASIS, SC or *.PSIS
      const auto& L_ASIS = landmarks["L.ASIS"];
      const auto& R_ASIS = landmarks["R.ASIS"];
      const auto& L_PSIS = landmarks["L.PSIS"];
      const auto& R_PSIS = landmarks["R.PSIS"];
      math::Position SC = landmarks["SC"];
      if (!L_ASIS.isValid() || !R_ASIS.isValid())
      {
        error("PluginGait - Missing landmarks (ASISs) to define the pelvis. Movement reconstruction aborted for trial '%s'.", trial->name().c_str());
        return false;
      }
      if (!SC.isValid() && (!L_PSIS.isValid() || !R_PSIS.isValid()))
      {
        error("PluginGait - Missing landmarks (SC or PSISs) to define the pelvis. Movement reconstruction aborted for trial '%s'.", trial->name().c_str());
        return false;
      }
      if (!SC.isValid())
        SC = (L_PSIS + R_PSIS) / 2.0;
      ReferenceFrame* relframe = nullptr;
      if ((relframe = this->findChild<ReferenceFrame*>("Pelvis.SCS",{},false)) == nullptr)
      {
        error("InternationalSocietyBiomechanics - Relative pelvis segment coordinate system not found. Did you calibrate first the helper? Movement reconstruction aborted.");
        return false;
      }
      const math::Vector w = (R_ASIS - L_ASIS).normalized();
      const math::Vector v = w.cross((R_ASIS + L_ASIS) / 2.0 - SC).normalized();
      const math::Pose pelvis(v.cross(w), v, w, SC);
      math::to_timesequence(transform_relative_frame(relframe, seg, math::Pose(v.cross(w),v,w,SC)), seg->name()+".SCS", sampleRate, startTime, TimeSequence::Pose, "", relframe);
      seg->setProperty("length", this->property(seg->name()+".length"));
      // -----------------------------------------
      // Thigh, shank, foot
      // -----------------------------------------
      if ((optr->Side & Side::Left) == Side::Left)
      {
        if (!optr->reconstructLowerLimb(model, trial, Side::Left, &landmarks, sampleRate, startTime))
          return false;
      }
      if ((optr->Side & Side::Right) == Side::Right)
      {
        if (!optr->reconstructLowerLimb(model, trial, Side::Right, &landmarks, sampleRate, startTime))
          return false;
      }
    }
    return true;
  };
  
  /**
   * Returns the internal parameter LeftStaticPlantarFlexionOffset.
   */
  Sex InternationalSocietyBiomechanics::sex() const _OPENMA_NOEXCEPT
  {
    auto optr = this->pimpl();
    return optr->Sex_;
  };
  
  /**
   * Returns the internal parameter LeftStaticRotationOffset.
   */
  void InternationalSocietyBiomechanics::setSex(Sex value) _OPENMA_NOEXCEPT
  {
    auto optr = this->pimpl();
    if (optr->Sex_ == value)
      return;
    optr->Sex_ = value;
    this->modified();
  };
  
  // ----------------------------------------------------------------------- //
  
  /*
   * Generate a landmarks translator for the InternationalSocietyBiomechanics model.
   * The following list presents the label used in the InternationalSocietyBiomechanics helper. The default translation uses the same internal and external names as this model was developed within OpenMA and not by a third-party company.
   *  - C7:     C7     (Seventh cervical vertebra: also named vertebra)
   *  - T8:     T8     (Spinous Process of the 8th thoracic vertebrae)
   *  - SS:     SS     (Suprasternal notch: superior border of the manubrium of the sternum, also known as the jugular notch)
   *  - XP:     XP     (Xiphoid process: extension to the lower part of the sternum)
   *  - L.AC:   L.AC   (Left acromion - Acromio-clavicular joint)
   *  - L.GH:   L.GH   (Left glenohumeral head - Glenohumeral joint center projected on the lateral side of the arm)
   *  - L.LHE:  L.LHE  (Left lateral humerus epicondyle)
   *  - L.MHE:  L.MHE  (Left medial humerus epicondyle)
   *  - L.US:   L.US   (Left ulnar styloid process)
   *  - L.RS:   L.RS   (Left radius styloid process)
   *  - L.MH5:  L.MH5  (Left head of the fifth metacarpus)
   *  - L.MH2:  L.MH2  (Left head of the second metacarpus)
   *  - R.AC:   R.AC   (Right acromion - Acromio-clavicular joint)
   *  - R.GH:   R.GH   (Right glenohumeral head - Glenohumeral joint center projected on the lateral side of the arm)
   *  - R.LHE:  R.LHE  (Right lateral humerus epicondyle)
   *  - R.MHE:  R.MHE  (Right medial humerus epicondyle)
   *  - R.US:   R.US   (Right ulnar styloid process)
   *  - R.RS:   R.RS   (Right radius styloid process)
   *  - R.MH5:  R.MH5  (Right head of the fifth metacarpus)
   *  - R.MH2:  R.MH2  (Right head of the second metacarpus)
   *  - SC:     SC     (Midpoint between Left and right PSIS)
   *  - L.ASIS: L.ASIS (Left Anterior Superior Iliac Spine)
   *  - L.PSIS: L.PSIS (Right Anterior Superior Iliac Spine)
   *  - R.ASIS: R.ASIS (Left Posterior Superior Iliac Spine)
   *  - R.PSIS: R.PSIS (Right Posterior Superior Iliac Spine)
   *  - L.GT:   L.GT   (Left Greater Trochanter)
   *  - L.LFE:  L.LFE  (Left lateral epicondyle of the femur)
   *  - L.MFE:  L.MFE  (Left medial epicondyle of the femur)
   *  - L.FH:   L.FH   (Left Fibula Head)
   *  - L.LTM:  L.LTM  (Left lateral malleolus of the tibia)
   *  - L.MTM:  L.MTM  (Left medial malleolus of the tibia)
   *  - L.MTH1: L.MTH1 (Left head of the first metatarsal)
   *  - L.MTH5: L.MTH5 (Left head of the fifth metatarsal)
   *  - L.HEE:  L.HEE  (Left heel marker - Calcaneous)
   *  - R.GT:   R.GT   (Right Greater Trochanter)
   *  - R.LFE:  R.LFE  (Right lateral epicondyle of the femur)
   *  - R.MFE:  R.MFE  (Right medial epicondyle of the femur)
   *  - R.FH:   R.FH   (Right Fibula Head)
   *  - R.LTM:  R.LTM  (Right lateral malleolus of the tibia)
   *  - R.MTM:  R.MTM  (Right medial malleolus of the tibia)
   *  - R.MTH1: R.MTH1 (Right head of the first metatarsal)
   *  - R.MTH5: R.MTH5 (Right head of the fifth metatarsal)
   *  - R.HEE:  R.HEE  (Right heel marker - Calcaneous)
   */
  LandmarksTranslator* InternationalSocietyBiomechanics::defaultLandmarksTranslator()
  {
    return new LandmarksTranslator(
      "LandmarksTranslator", 
      {
        // Torso
        {"C7", "C7"},
        {"T8", "T8"},
        {"SS", "SS"},
        {"XP", "XP"},
        // Left arm
        {"L.AC", "L.AC"},
        {"L.GH", "L.GH"},
        {"L.LHE", "L.LHE"},
        {"L.MHE", "L.MHE"},
        // Left forearm
        {"L.US", "L.US"},
        {"L.RS", "L.RS"},
        // Left hand
        {"L.MH5", "L.MH5"},
        {"L.MH2", "L.MH2"},
        // Right arm
        {"R.AC", "R.AC"},
        {"R.GH", "R.GH"},
        {"R.LHE", "R.LHE"},
        {"R.MHE", "R.MHE"},
        // Right forearm
        {"R.US", "R.US"},
        {"R.RS", "R.RS"},
        // Right hand
        {"R.MH5", "R.MH5"},
        {"R.MH2", "R.MH2"},
        // Pelvis
        {"SC", "SC"},
        {"L.ASIS", "L.ASIS"},
        {"L.PSIS", "L.PSIS"},
        {"R.ASIS", "R.ASIS"},
        {"R.PSIS", "R.PSIS"},
        // Left thigh
        {"L.GT", "L.GT"},
        {"L.LFE", "L.LFE"},
        {"L.MFE", "L.MFE"},
        // Left shank
        {"L.FH", "L.FH"},
        {"L.LTM", "L.LTM"},
        {"L.MTM", "L.MTM"},
        // Left foot
        {"L.MTH1", "L.MTH1"},
        {"L.MTH5", "L.MTH5"},
        {"L.HEE", "L.HEE"},
        // Right thigh
        {"R.GT", "R.GT"},
        {"R.LFE", "R.LFE"},
        {"R.MFE", "R.MFE"},
        // Right shank
        {"R.FH", "R.FH"},
        {"R.LTM", "R.LTM"},
        {"R.MTM", "R.MTM"},
        // Right foot
        {"R.MTH1", "R.MTH1"},
        {"R.MTH5", "R.MTH5"},
        {"R.HEE", "R.HEE"},
      }, this);
  };
  
  /**
   *
   */
  InertialParametersEstimator* InternationalSocietyBiomechanics::defaultInertialParametersEstimator()
  {
    return new DumasMcConvilleYoungTable(this);
  };
  
  /**
   *
   */
  ExternalWrenchAssigner* InternationalSocietyBiomechanics::defaultExternalWrenchAssigner()
  {
    return nullptr;
  };
  
  /**
   *
   */
  InverseDynamicProcessor* InternationalSocietyBiomechanics::defaultInverseDynamicProcessor()
  {
    return new InverseDynamicMatrix(this);
  };
  
  /**
   * Create a deep copy of the object and return it as another object.
   */
  InternationalSocietyBiomechanics* InternationalSocietyBiomechanics::clone(Node* parent) const
  {
    auto dest = new InternationalSocietyBiomechanics(0,0);
    dest->copy(this);
    dest->addParent(parent);
    return dest;
  };
  
  /**
   * Do a deep copy of the the given @a source. The previous content is replaced.
   */
  void InternationalSocietyBiomechanics::copy(const Node* source) _OPENMA_NOEXCEPT
  {
    auto src = node_cast<const InternationalSocietyBiomechanics*>(source);
    if (src == nullptr)
      return;
    auto optr = this->pimpl();
    auto optr_src = src->pimpl();
    this->SkeletonHelper::copy(src);
    optr->Sex_ = optr_src->Sex_;
  };
};
};