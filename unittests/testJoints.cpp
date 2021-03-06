/*
 * Copyright (c) 2013-2015, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Jeongseok Lee <jslee02@gmail.com>
 *
 * Georgia Tech Graphics Lab and Humanoid Robotics Lab
 *
 * Directed by Prof. C. Karen Liu and Prof. Mike Stilman
 * <karenliu@cc.gatech.edu> <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include <array>
#include <iostream>
#include <gtest/gtest.h>
#include "TestHelpers.h"

#include "dart/math/Geometry.h"
#include "dart/math/Helpers.h"
#include "dart/dynamics/BallJoint.h"
#include "dart/dynamics/FreeJoint.h"
#include "dart/dynamics/PrismaticJoint.h"
#include "dart/dynamics/RevoluteJoint.h"
#include "dart/dynamics/TranslationalJoint.h"
#include "dart/dynamics/UniversalJoint.h"
#include "dart/dynamics/WeldJoint.h"
#include "dart/dynamics/EulerJoint.h"
#include "dart/dynamics/ScrewJoint.h"
#include "dart/dynamics/PlanarJoint.h"
#include "dart/dynamics/BodyNode.h"
#include "dart/dynamics/Skeleton.h"
#include "dart/simulation/World.h"
#include "dart/utils/SkelParser.h"

using namespace dart;
using namespace dart::math;
using namespace dart::dynamics;
using namespace dart::simulation;

#define JOINT_TOL 0.01

//==============================================================================
class JOINTS : public testing::Test
{
public:
  // Get reference frames
  const std::vector<SimpleFrame*>& getFrames() const;

  // Randomize the properties of all the reference frames
  void randomizeRefFrames();

#ifdef _WIN32
  template <typename JointType>
  static typename JointType::Properties createJointProperties()
  {
    return typename JointType::Properties();
  }
#endif

  template <typename JointType>
  void kinematicsTest(
#ifdef _WIN32
      const typename JointType::Properties& _joint
          = BodyNode::createJointProperties<JointType>());
#else
      const typename JointType::Properties& _joint
          = typename JointType::Properties());
#endif

protected:
  // Sets up the test fixture.
  virtual void SetUp();

  std::vector<SimpleFrame*> frames;
};

//==============================================================================
void JOINTS::SetUp()
{
  // Create a list of reference frames to use during tests
  frames.push_back(new SimpleFrame(Frame::World(), "refFrame1"));
  frames.push_back(new SimpleFrame(frames.back(), "refFrame2"));
  frames.push_back(new SimpleFrame(frames.back(), "refFrame3"));
  frames.push_back(new SimpleFrame(frames.back(), "refFrame4"));
  frames.push_back(new SimpleFrame(Frame::World(), "refFrame5"));
  frames.push_back(new SimpleFrame(frames.back(), "refFrame6"));
}

//==============================================================================
const std::vector<SimpleFrame*>& JOINTS::getFrames() const
{
  return frames;
}

//==============================================================================
void JOINTS::randomizeRefFrames()
{
  for(size_t i=0; i<frames.size(); ++i)
  {
    SimpleFrame* F = frames[i];

    Eigen::Vector3d p = randomVector<3>(100);
    Eigen::Vector3d theta = randomVector<3>(2*M_PI);

    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translate(p);
    tf.linear() = math::eulerXYZToMatrix(theta);

    F->setRelativeTransform(tf);
    F->setRelativeSpatialVelocity(randomVector<6>(100));
    F->setRelativeSpatialAcceleration(randomVector<6>(100));
  }
}

//==============================================================================
template <typename JointType>
void JOINTS::kinematicsTest(const typename JointType::Properties& _properties)
{
  int numTests = 1;

  SkeletonPtr skeleton = Skeleton::create();
  Joint* joint = skeleton->createJointAndBodyNodePair<JointType>(
        nullptr, _properties).first;
  joint->setTransformFromChildBodyNode(math::expMap(Eigen::Vector6d::Random()));
  joint->setTransformFromParentBodyNode(math::expMap(Eigen::Vector6d::Random()));

  int dof = joint->getNumDofs();

  //--------------------------------------------------------------------------
  //
  //--------------------------------------------------------------------------
  VectorXd q = VectorXd::Zero(dof);
  VectorXd dq = VectorXd::Zero(dof);

  for (int idxTest = 0; idxTest < numTests; ++idxTest)
  {
    double q_delta = 0.000001;

    for (int i = 0; i < dof; ++i)
    {
      q(i) = random(-DART_PI*1.0, DART_PI*1.0);
      dq(i) = random(-DART_PI*1.0, DART_PI*1.0);
    }

    skeleton->setPositions(q);
    skeleton->setVelocities(dq);

    if (dof == 0)
      return;

    Eigen::Isometry3d T = joint->getLocalTransform();
    Jacobian J = joint->getLocalJacobian();
    Jacobian dJ = joint->getLocalJacobianTimeDeriv();

    //--------------------------------------------------------------------------
    // Test T
    //--------------------------------------------------------------------------
    EXPECT_TRUE(math::verifyTransform(T));

    //--------------------------------------------------------------------------
    // Test analytic Jacobian and numerical Jacobian
    // J == numericalJ
    //--------------------------------------------------------------------------
    Jacobian numericJ = Jacobian::Zero(6,dof);
    for (int i = 0; i < dof; ++i)
    {
      // a
      Eigen::VectorXd q_a = q;
      joint->setPositions(q_a);
      Eigen::Isometry3d T_a = joint->getLocalTransform();

      // b
      Eigen::VectorXd q_b = q;
      q_b(i) += q_delta;
      joint->setPositions(q_b);
      Eigen::Isometry3d T_b = joint->getLocalTransform();

      //
      Eigen::Isometry3d Tinv_a = T_a.inverse();
      Eigen::Matrix4d Tinv_a_eigen = Tinv_a.matrix();

      // dTdq
      Eigen::Matrix4d T_a_eigen = T_a.matrix();
      Eigen::Matrix4d T_b_eigen = T_b.matrix();
      Eigen::Matrix4d dTdq_eigen = (T_b_eigen - T_a_eigen) / q_delta;
      //Matrix4d dTdq_eigen = (T_b_eigen * T_a_eigen.inverse()) / dt;

      // J(i)
      Eigen::Matrix4d Ji_4x4matrix_eigen = Tinv_a_eigen * dTdq_eigen;
      Eigen::Vector6d Ji;
      Ji[0] = Ji_4x4matrix_eigen(2,1);
      Ji[1] = Ji_4x4matrix_eigen(0,2);
      Ji[2] = Ji_4x4matrix_eigen(1,0);
      Ji[3] = Ji_4x4matrix_eigen(0,3);
      Ji[4] = Ji_4x4matrix_eigen(1,3);
      Ji[5] = Ji_4x4matrix_eigen(2,3);
      numericJ.col(i) = Ji;
    }

    for (int i = 0; i < dof; ++i)
    {
      for (int j = 0; j < 6; ++j)
        EXPECT_NEAR(J.col(i)(j), numericJ.col(i)(j), JOINT_TOL);
    }

    //--------------------------------------------------------------------------
    // Test first time derivative of analytic Jacobian and numerical Jacobian
    // dJ == numerical_dJ
    //--------------------------------------------------------------------------
    Jacobian numeric_dJ = Jacobian::Zero(6,dof);
    for (int i = 0; i < dof; ++i)
    {
      // a
      Eigen::VectorXd q_a = q;
      joint->setPositions(q_a);
      Jacobian J_a = joint->getLocalJacobian();

      // b
      Eigen::VectorXd q_b = q;
      q_b(i) += q_delta;
      joint->setPositions(q_b);
      Jacobian J_b = joint->getLocalJacobian();

      //
      Jacobian dJ_dq = (J_b - J_a) / q_delta;

      // J(i)
      numeric_dJ += dJ_dq * dq(i);
    }

    for (int i = 0; i < dof; ++i)
    {
      for (int j = 0; j < 6; ++j)
        EXPECT_NEAR(dJ.col(i)(j), numeric_dJ.col(i)(j), JOINT_TOL);
    }
  }

  // Forward kinematics test with high joint position
  double posMin = -1e+64;
  double posMax = +1e+64;

  for (int idxTest = 0; idxTest < numTests; ++idxTest)
  {
    for (int i = 0; i < dof; ++i)
      q(i) = random(posMin, posMax);

    skeleton->setPositions(q);

    if (joint->getNumDofs() == 0)
      return;

    Eigen::Isometry3d T = joint->getLocalTransform();
    EXPECT_TRUE(math::verifyTransform(T));
  }
}

// 0-dof joint
TEST_F(JOINTS, WELD_JOINT)
{
  kinematicsTest<WeldJoint>();
}

// 1-dof joint
TEST_F(JOINTS, REVOLUTE_JOINT)
{
  kinematicsTest<RevoluteJoint>();
}

// 1-dof joint
TEST_F(JOINTS, PRISMATIC_JOINT)
{
  kinematicsTest<PrismaticJoint>();
}

// 1-dof joint
TEST_F(JOINTS, SCREW_JOINT)
{
  kinematicsTest<ScrewJoint>();
}

// 2-dof joint
TEST_F(JOINTS, UNIVERSAL_JOINT)
{
  kinematicsTest<UniversalJoint>();
}

// 3-dof joint
//TEST_F(JOINTS, BALL_JOINT)
//{
//  kinematicsTest<BallJoint>();
//}
// TODO(JS): Disabled the test compares analytical Jacobian and numerical
// Jacobian since the meaning of BallJoint Jacobian is changed per
// we now use angular velocity and angular accertions as BallJoint's generalized
// velocities and accelerations, repectively.

// 3-dof joint
TEST_F(JOINTS, EULER_JOINT)
{
  EulerJoint::Properties properties;

  properties.mAxisOrder = EulerJoint::AO_XYZ;
  kinematicsTest<EulerJoint>(properties);

  properties.mAxisOrder = EulerJoint::AO_ZYX;
  kinematicsTest<EulerJoint>(properties);
}

// 3-dof joint
TEST_F(JOINTS, TRANSLATIONAL_JOINT)
{
  kinematicsTest<TranslationalJoint>();
}

// 3-dof joint
TEST_F(JOINTS, PLANAR_JOINT)
{
  kinematicsTest<PlanarJoint>();
}

// 6-dof joint
//TEST_F(JOINTS, FREE_JOINT)
//{
//  kinematicsTest<FreeJoint>();
//}
// TODO(JS): Disabled the test compares analytical Jacobian and numerical
// Jacobian since the meaning of FreeJoint Jacobian is changed per
// we now use spatial velocity and spatial accertions as FreeJoint's generalized
// velocities and accelerations, repectively.

//==============================================================================
template <void (Joint::*setX)(std::size_t, double),
          void (Joint::*setXLowerLimit)(std::size_t, double),
          void (Joint::*setXUpperLimit)(std::size_t, double)>
void testCommandLimits(dynamics::Joint* joint)
{
  const double lower = -5.0;
  const double upper = +5.0;
  const double mid = 0.5 * (lower + upper);
  const double lessThanLower    = -10.0;
  const double greaterThanUpper = +10.0;

  for (std::size_t i = 0; i < joint->getNumDofs(); ++i)
  {
    (joint->*setXLowerLimit)(i, lower);
    (joint->*setXUpperLimit)(i, upper);

    joint->setCommand(i, mid);
    EXPECT_EQ(joint->getCommand(i), mid);
    (joint->*setX)(i, mid);
    EXPECT_EQ(joint->getCommand(i), mid);

    joint->setCommand(i, lessThanLower);
    EXPECT_EQ(joint->getCommand(i), lower);
    (joint->*setX)(i, lessThanLower);
    EXPECT_EQ(joint->getCommand(i), lessThanLower);

    joint->setCommand(i, greaterThanUpper);
    EXPECT_EQ(joint->getCommand(i), upper);
    (joint->*setX)(i, greaterThanUpper);
    EXPECT_EQ(joint->getCommand(i), greaterThanUpper);
  }
}

//==============================================================================
TEST_F(JOINTS, COMMAND_LIMIT)
{
  simulation::WorldPtr myWorld
      = utils::SkelParser::readWorld(
        DART_DATA_PATH"/skel/test/joint_limit_test.skel");
  EXPECT_TRUE(myWorld != nullptr);

  dynamics::SkeletonPtr pendulum = myWorld->getSkeleton("double_pendulum");
  EXPECT_TRUE(pendulum != nullptr);

  auto bodyNodes = pendulum->getBodyNodes();

  for (auto bodyNode : bodyNodes)
  {
    Joint* joint = bodyNode->getParentJoint();

    joint->setActuatorType(Joint::FORCE);
    EXPECT_EQ(joint->getActuatorType(), Joint::FORCE);
    testCommandLimits<
        &Joint::setForce,
        &Joint::setForceLowerLimit,
        &Joint::setForceUpperLimit>(joint);

    joint->setActuatorType(Joint::ACCELERATION);
    EXPECT_EQ(joint->getActuatorType(), Joint::ACCELERATION);
    testCommandLimits<
        &Joint::setAcceleration,
        &Joint::setAccelerationLowerLimit,
        &Joint::setAccelerationUpperLimit>(joint);

    joint->setActuatorType(Joint::VELOCITY);
    EXPECT_EQ(joint->getActuatorType(), Joint::VELOCITY);
    testCommandLimits<
        &Joint::setVelocity,
        &Joint::setVelocityLowerLimit,
        &Joint::setVelocityUpperLimit>(joint);
  }
}

//==============================================================================
TEST_F(JOINTS, POSITION_LIMIT)
{
  double tol = 1e-3;

  simulation::WorldPtr myWorld
      = utils::SkelParser::readWorld(
        DART_DATA_PATH"/skel/test/joint_limit_test.skel");
  EXPECT_TRUE(myWorld != nullptr);

  myWorld->setGravity(Eigen::Vector3d(0.0, 0.0, 0.0));

  dynamics::SkeletonPtr pendulum = myWorld->getSkeleton("double_pendulum");
  EXPECT_TRUE(pendulum != nullptr);

  dynamics::Joint* joint0 = pendulum->getJoint("joint0");
  dynamics::Joint* joint1 = pendulum->getJoint("joint1");

  EXPECT_TRUE(joint0 != nullptr);
  EXPECT_TRUE(joint1 != nullptr);

  double limit0 = DART_PI / 6.0;
  double limit1 = DART_PI / 6.0;

  joint0->setPositionLimitEnforced(true);
  joint0->setPositionLowerLimit(0, -limit0);
  joint0->setPositionUpperLimit(0, limit0);

  joint1->setPositionLimitEnforced(true);
  joint1->setPositionLowerLimit(0, -limit1);
  joint1->setPositionUpperLimit(0, limit1);

#ifndef NDEBUG // Debug mode
  double simTime = 0.2;
#else
  double simTime = 2.0;
#endif // ------- Debug mode
  double timeStep = myWorld->getTimeStep();
  int nSteps = simTime / timeStep;

  // Two seconds with positive control forces
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, 0.1);
    joint1->setForce(0, 0.1);
    myWorld->step();

    double jointPos0 = joint0->getPosition(0);
    double jointPos1 = joint1->getPosition(0);

    EXPECT_GE(jointPos0, -limit0 - tol);
    EXPECT_GE(jointPos1, -limit1 - tol);

    EXPECT_LE(jointPos0, limit0 + tol);
    EXPECT_LE(jointPos1, limit1 + tol);
  }

  // Two more seconds with negative control forces
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, -0.1);
    joint1->setForce(0, -0.1);
    myWorld->step();

    double jointPos0 = joint0->getPosition(0);
    double jointPos1 = joint1->getPosition(0);

    EXPECT_GE(jointPos0, -limit0 - tol);
    EXPECT_GE(jointPos1, -limit1 - tol);

    EXPECT_LE(jointPos0, limit0 + tol);
    EXPECT_LE(jointPos1, limit1 + tol);
  }
}

//==============================================================================
void testJointCoulombFrictionForce(double _timeStep)
{
  double tol = 1e-9;

  simulation::WorldPtr myWorld
      = utils::SkelParser::readWorld(
        DART_DATA_PATH"/skel/test/joint_friction_test.skel");
  EXPECT_TRUE(myWorld != nullptr);

  myWorld->setGravity(Eigen::Vector3d(0.0, 0.0, 0.0));
  myWorld->setTimeStep(_timeStep);

  dynamics::SkeletonPtr pendulum = myWorld->getSkeleton("double_pendulum");
  EXPECT_TRUE(pendulum != nullptr);
  pendulum->disableSelfCollision();

  dynamics::Joint* joint0 = pendulum->getJoint("joint0");
  dynamics::Joint* joint1 = pendulum->getJoint("joint1");

  EXPECT_TRUE(joint0 != nullptr);
  EXPECT_TRUE(joint1 != nullptr);

  double frictionForce  = 5.0;

  joint0->setPositionLimitEnforced(false);
  joint1->setPositionLimitEnforced(false);

  joint0->setCoulombFriction(0, frictionForce);
  joint1->setCoulombFriction(0, frictionForce);

  EXPECT_EQ(joint0->getCoulombFriction(0), frictionForce);
  EXPECT_EQ(joint1->getCoulombFriction(0), frictionForce);

#ifndef NDEBUG // Debug mode
  double simTime = 0.2;
#else
  double simTime = 2.0;
#endif // ------- Debug mode
  double timeStep = myWorld->getTimeStep();
  int nSteps = simTime / timeStep;

  // Two seconds with lower control forces than the friction
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, +4.9);
    joint1->setForce(0, +4.9);
    myWorld->step();

    double jointVel0 = joint0->getVelocity(0);
    double jointVel1 = joint1->getVelocity(0);

    EXPECT_NEAR(jointVel0, 0.0, tol);
    EXPECT_NEAR(jointVel1, 0.0, tol);
  }

  // Another two seconds with lower control forces than the friction forces
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, -4.9);
    joint1->setForce(0, -4.9);
    myWorld->step();

    double jointVel0 = joint0->getVelocity(0);
    double jointVel1 = joint1->getVelocity(0);

    EXPECT_NEAR(jointVel0, 0.0, tol);
    EXPECT_NEAR(jointVel1, 0.0, tol);
  }

  // Another two seconds with higher control forces than the friction forces
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, 10.0);
    joint1->setForce(0, 10.0);
    myWorld->step();

    double jointVel0 = joint0->getVelocity(0);
    double jointVel1 = joint1->getVelocity(0);

    EXPECT_GE(std::abs(jointVel0), 0.0);
    EXPECT_GE(std::abs(jointVel1), 0.0);
  }

  // Spend 20 sec waiting the joints to stop
  for (int i = 0; i < nSteps * 10; i++)
  {
    myWorld->step();
  }
  double jointVel0 = joint0->getVelocity(0);
  double jointVel1 = joint1->getVelocity(0);

  EXPECT_NEAR(jointVel0, 0.0, tol);
  EXPECT_NEAR(jointVel1, 0.0, tol);

  // Another two seconds with lower control forces than the friction forces
  // and expect the joints to stop
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, 4.9);
    joint1->setForce(0, 4.9);
    myWorld->step();

    double jointVel0 = joint0->getVelocity(0);
    double jointVel1 = joint1->getVelocity(0);

    EXPECT_NEAR(jointVel0, 0.0, tol);
    EXPECT_NEAR(jointVel1, 0.0, tol);
  }
}

//==============================================================================
TEST_F(JOINTS, JOINT_COULOMB_FRICTION)
{
  std::array<double, 3> timeSteps;
  timeSteps[0] = 1e-2;
  timeSteps[1] = 1e-3;
  timeSteps[2] = 1e-4;

  for (auto timeStep : timeSteps)
    testJointCoulombFrictionForce(timeStep);
}

//==============================================================================
TEST_F(JOINTS, JOINT_COULOMB_FRICTION_AND_POSITION_LIMIT)
{
  const double timeStep = 1e-3;
  const double tol = 1e-2;

  simulation::WorldPtr myWorld
      = utils::SkelParser::readWorld(
        DART_DATA_PATH"/skel/test/joint_friction_test.skel");
  EXPECT_TRUE(myWorld != nullptr);

  myWorld->setGravity(Eigen::Vector3d(0.0, 0.0, 0.0));
  myWorld->setTimeStep(timeStep);

  dynamics::SkeletonPtr pendulum = myWorld->getSkeleton("double_pendulum");
  EXPECT_TRUE(pendulum != nullptr);
  pendulum->disableSelfCollision();

  dynamics::Joint* joint0 = pendulum->getJoint("joint0");
  dynamics::Joint* joint1 = pendulum->getJoint("joint1");

  EXPECT_TRUE(joint0 != nullptr);
  EXPECT_TRUE(joint1 != nullptr);

  double frictionForce  = 5.0;

  joint0->setPositionLimitEnforced(true);
  joint1->setPositionLimitEnforced(true);

  const double ll = -DART_PI/12.0; // -15 degree
  const double ul = +DART_PI/12.0; // +15 degree

  size_t dof0 = joint0->getNumDofs();
  for (size_t i = 0; i < dof0; ++i)
  {
    joint0->setPosition(i, 0.0);
    joint0->setPosition(i, 0.0);
    joint0->setPositionLowerLimit(i, ll);
    joint0->setPositionUpperLimit(i, ul);
  }

  size_t dof1 = joint1->getNumDofs();
  for (size_t i = 0; i < dof1; ++i)
  {
    joint1->setPosition(i, 0.0);
    joint1->setPosition(i, 0.0);
    joint1->setPositionLowerLimit(i, ll);
    joint1->setPositionUpperLimit(i, ul);
  }

  joint0->setCoulombFriction(0, frictionForce);
  joint1->setCoulombFriction(0, frictionForce);

  EXPECT_EQ(joint0->getCoulombFriction(0), frictionForce);
  EXPECT_EQ(joint1->getCoulombFriction(0), frictionForce);

#ifndef NDEBUG // Debug mode
  double simTime = 0.2;
#else
  double simTime = 2.0;
#endif // ------- Debug mode
  int nSteps = simTime / timeStep;

  // First two seconds rotating in positive direction with higher control forces
  // than the friction forces
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, 100.0);
    joint1->setForce(0, 100.0);
    myWorld->step();

    double jointPos0 = joint0->getPosition(0);
    double jointPos1 = joint1->getPosition(0);

    double jointVel0 = joint0->getVelocity(0);
    double jointVel1 = joint1->getVelocity(0);

    EXPECT_GE(std::abs(jointVel0), 0.0);
    EXPECT_GE(std::abs(jointVel1), 0.0);

    EXPECT_GE(jointPos0, ll - tol);
    EXPECT_LE(jointPos0, ul + tol);

    EXPECT_GE(jointPos1, ll - tol);
    EXPECT_LE(jointPos1, ul + tol);
  }

  // Another two seconds rotating in negative direction with higher control
  // forces than the friction forces
  for (int i = 0; i < nSteps; i++)
  {
    joint0->setForce(0, -100.0);
    joint1->setForce(0, -100.0);
    myWorld->step();

    double jointPos0 = joint0->getPosition(0);
    double jointPos1 = joint1->getPosition(0);

    double jointVel0 = joint0->getVelocity(0);
    double jointVel1 = joint1->getVelocity(0);

    EXPECT_GE(std::abs(jointVel0), 0.0);
    EXPECT_GE(std::abs(jointVel1), 0.0);

    EXPECT_GE(jointPos0, ll - tol);
    EXPECT_LE(jointPos0, ul + tol);

    EXPECT_GE(jointPos1, ll - tol);
    EXPECT_LE(jointPos1, ul + tol);
  }
}

//==============================================================================
template<int N>
Eigen::Matrix<double,N,1> random_vec(double limit=100)
{
  Eigen::Matrix<double,N,1> v;
  for(size_t i=0; i<N; ++i)
    v[i] = math::random(-std::abs(limit), std::abs(limit));
  return v;
}

//==============================================================================
Eigen::Isometry3d random_transform(double translation_limit=100,
                                   double rotation_limit=2*M_PI)
{
  Eigen::Vector3d r = random_vec<3>(translation_limit);
  Eigen::Vector3d theta = random_vec<3>(rotation_limit);

  Eigen::Isometry3d tf;
  tf.setIdentity();
  tf.translate(r);

  if(theta.norm()>0)
    tf.rotate(Eigen::AngleAxisd(theta.norm(), theta.normalized()));

  return tf;
}

//==============================================================================
Eigen::Isometry3d predict_joint_transform(Joint* joint,
                                          const Eigen::Isometry3d& joint_tf)
{
  return joint->getTransformFromParentBodyNode() * joint_tf
          * joint->getTransformFromChildBodyNode().inverse();
}

//==============================================================================
Eigen::Isometry3d get_relative_transform(BodyNode* bn, BodyNode* relativeTo)
{
  return relativeTo->getTransform().inverse() * bn->getTransform();
}

//==============================================================================
TEST_F(JOINTS, CONVENIENCE_FUNCTIONS)
{
  SkeletonPtr skel = Skeleton::create();

  std::pair<Joint*, BodyNode*> pair;

  pair = skel->createJointAndBodyNodePair<WeldJoint>();
  BodyNode* root = pair.second;

  // -- set up the FreeJoint
  std::pair<FreeJoint*, BodyNode*> freepair =
      root->createChildJointAndBodyNodePair<FreeJoint>();
  FreeJoint* freejoint = freepair.first;
  BodyNode* freejoint_bn = freepair.second;

  freejoint->setTransformFromParentBodyNode(random_transform());
  freejoint->setTransformFromChildBodyNode(random_transform());

  // -- set up the EulerJoint
  std::pair<EulerJoint*, BodyNode*> eulerpair =
      root->createChildJointAndBodyNodePair<EulerJoint>();
  EulerJoint* eulerjoint = eulerpair.first;
  BodyNode* eulerjoint_bn = eulerpair.second;

  eulerjoint->setTransformFromParentBodyNode(random_transform());
  eulerjoint->setTransformFromChildBodyNode(random_transform());

  // -- set up the BallJoint
  std::pair<BallJoint*, BodyNode*> ballpair =
      root->createChildJointAndBodyNodePair<BallJoint>();
  BallJoint* balljoint = ballpair.first;
  BodyNode* balljoint_bn = ballpair.second;

  balljoint->setTransformFromParentBodyNode(random_transform());
  balljoint->setTransformFromChildBodyNode(random_transform());

  // Test a hundred times
  for(size_t n=0; n<100; ++n)
  {
    // -- convert transforms to positions and then positions back to transforms
    Eigen::Isometry3d desired_freejoint_tf = random_transform();
    freejoint->setPositions(FreeJoint::convertToPositions(desired_freejoint_tf));
    Eigen::Isometry3d actual_freejoint_tf = FreeJoint::convertToTransform(
          freejoint->getPositions());

    Eigen::Isometry3d desired_eulerjoint_tf = random_transform();
    desired_eulerjoint_tf.translation() = Eigen::Vector3d::Zero();
    eulerjoint->setPositions(
          eulerjoint->convertToPositions(desired_eulerjoint_tf.linear()));
    Eigen::Isometry3d actual_eulerjoint_tf = eulerjoint->convertToTransform(
          eulerjoint->getPositions());

    Eigen::Isometry3d desired_balljoint_tf = random_transform();
    desired_balljoint_tf.translation() = Eigen::Vector3d::Zero();
    balljoint->setPositions(
          BallJoint::convertToPositions(desired_balljoint_tf.linear()));
    Eigen::Isometry3d actual_balljoint_tf = BallJoint::convertToTransform(
          balljoint->getPositions());

    // -- collect everything so we can cycle through the tests
    std::vector<Joint*> joints;
    std::vector<BodyNode*> bns;
    Eigen::aligned_vector<Eigen::Isometry3d> desired_tfs;
    Eigen::aligned_vector<Eigen::Isometry3d> actual_tfs;

    joints.push_back(freejoint);
    bns.push_back(freejoint_bn);
    desired_tfs.push_back(desired_freejoint_tf);
    actual_tfs.push_back(actual_freejoint_tf);

    joints.push_back(eulerjoint);
    bns.push_back(eulerjoint_bn);
    desired_tfs.push_back(desired_eulerjoint_tf);
    actual_tfs.push_back(actual_eulerjoint_tf);

    joints.push_back(balljoint);
    bns.push_back(balljoint_bn);
    desired_tfs.push_back(desired_balljoint_tf);
    actual_tfs.push_back(actual_balljoint_tf);

    for(size_t i=0; i<joints.size(); ++i)
    {
      Joint* joint = joints[i];
      BodyNode* bn = bns[i];
      Eigen::Isometry3d tf = desired_tfs[i];

      bool check_transform_conversion =
          equals(predict_joint_transform(joint, tf).matrix(),
                 get_relative_transform(bn, bn->getParentBodyNode()).matrix());
      EXPECT_TRUE(check_transform_conversion);

      if(!check_transform_conversion)
      {
        std::cout << "[" << joint->getName() << " Failed]\n";
        std::cout << "Predicted:\n" << predict_joint_transform(joint, tf).matrix()
                  << "\n\nActual:\n"
                  << get_relative_transform(bn, bn->getParentBodyNode()).matrix()
                  << "\n\n";
      }

      bool check_full_cycle = equals(desired_tfs[i].matrix(),
                                     actual_tfs[i].matrix());
      EXPECT_TRUE(check_full_cycle);

      if(!check_full_cycle)
      {
        std::cout << "[" << joint->getName() << " Failed]\n";
        std::cout << "Desired:\n" << desired_tfs[i].matrix()
                  << "\n\nActual:\n" << actual_tfs[i].matrix()
                  << "\n\n";
      }
    }
  }
}

//==============================================================================
TEST_F(JOINTS, FREE_JOINT_RELATIVE_TRANSFORM_VELOCITY_ACCELERATION)
{
  const std::size_t numTests = 50;

  // Generate random reference frames
  randomizeRefFrames();
  auto refFrames = getFrames();

  // Generate random relative frames
  randomizeRefFrames();
  auto relFrames = getFrames();

  //-- Build a skeleton that contains two FreeJoints and two BodyNodes
  SkeletonPtr skel = Skeleton::create();

  auto pair = skel->createJointAndBodyNodePair<FreeJoint>();
  FreeJoint* rootJoint    = pair.first;
  BodyNode*  rootBodyNode = pair.second;
  rootJoint->setRelativeTransform(random_transform());
  rootJoint->setRelativeSpatialVelocity(random_vec<6>());
  rootJoint->setRelativeSpatialAcceleration(random_vec<6>());
  rootJoint->setTransformFromParentBodyNode(random_transform());
  rootJoint->setTransformFromChildBodyNode(random_transform());

  pair = rootBodyNode->createChildJointAndBodyNodePair<FreeJoint>();
  FreeJoint* joint1    = pair.first;
  BodyNode*  bodyNode1 = pair.second;
  joint1->setTransformFromParentBodyNode(random_transform());
  joint1->setTransformFromChildBodyNode(random_transform());

  //-- Actual terms
  Eigen::Isometry3d actualTf;
  Eigen::Vector6d   actualVel;
  Eigen::Vector6d   actualAcc;

  Eigen::Vector3d   actualLinVel;
  Eigen::Vector3d   actualAngVel;
  Eigen::Vector3d   actualLinAcc;
  Eigen::Vector3d   actualAngAcc;

  Eigen::Vector3d   oldLinVel;
  Eigen::Vector3d   oldAngVel;
  Eigen::Vector3d   oldLinAcc;
  Eigen::Vector3d   oldAngAcc;

  //-- Test
  for (std::size_t i = 0; i < numTests; ++i)
  {
    const Eigen::Isometry3d desiredTf     = random_transform();
    const Eigen::Vector6d   desiredVel    = random_vec<6>();
    const Eigen::Vector6d   desiredAcc    = random_vec<6>();
    const Eigen::Vector3d   desiredLinVel = random_vec<3>();
    const Eigen::Vector3d   desiredAngVel = random_vec<3>();
    const Eigen::Vector3d   desiredLinAcc = random_vec<3>();
    const Eigen::Vector3d   desiredAngAcc = random_vec<3>();

    //-- Relative transformation

    joint1->setRelativeTransform(desiredTf);
    actualTf = bodyNode1->getTransform(bodyNode1->getParentBodyNode());
    EXPECT_TRUE(equals(desiredTf.matrix(), actualTf.matrix()));

    for (auto relativeTo : relFrames)
    {
      joint1->setTransform(desiredTf, relativeTo);
      actualTf = bodyNode1->getTransform(relativeTo);
      EXPECT_TRUE(equals(desiredTf.matrix(), actualTf.matrix()));
    }

    //-- Relative spatial velocity

    joint1->setRelativeSpatialVelocity(desiredVel);
    actualVel = bodyNode1->getSpatialVelocity(
                  bodyNode1->getParentBodyNode(), bodyNode1);

    EXPECT_TRUE(equals(desiredVel, actualVel));

    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialVelocity(desiredVel, relativeTo, inCoordinatesOf);
        actualVel = bodyNode1->getSpatialVelocity(
                      relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(desiredVel, actualVel));
      }
    }

    //-- Relative classic linear velocity

    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialVelocity(desiredVel, relativeTo, inCoordinatesOf);
        oldAngVel
            = bodyNode1->getAngularVelocity(relativeTo, inCoordinatesOf);
        joint1->setLinearVelocity(desiredLinVel, relativeTo, inCoordinatesOf);

        actualLinVel
            = bodyNode1->getLinearVelocity(relativeTo, inCoordinatesOf);
        actualAngVel
            = bodyNode1->getAngularVelocity(relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(desiredLinVel, actualLinVel));
        EXPECT_TRUE(equals(oldAngVel, actualAngVel));
      }
    }

    //-- Relative classic angular velocity

    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialVelocity(desiredVel, relativeTo, inCoordinatesOf);
        oldLinVel
            = bodyNode1->getLinearVelocity(relativeTo, inCoordinatesOf);
        joint1->setAngularVelocity(desiredAngVel, relativeTo, inCoordinatesOf);

        actualLinVel =
            bodyNode1->getLinearVelocity(relativeTo, inCoordinatesOf);
        actualAngVel =
            bodyNode1->getAngularVelocity(relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(oldLinVel, actualLinVel));
        EXPECT_TRUE(equals(desiredAngVel, actualAngVel));
      }
    }

    //-- Relative spatial acceleration

    joint1->setRelativeSpatialAcceleration(desiredAcc);
    actualAcc = bodyNode1->getSpatialAcceleration(
                  bodyNode1->getParentBodyNode(), bodyNode1);

    EXPECT_TRUE(equals(desiredAcc, actualAcc));

    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialAcceleration(
              desiredAcc, relativeTo, inCoordinatesOf);
        actualAcc = bodyNode1->getSpatialAcceleration(
                      relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(desiredAcc, actualAcc));
      }
    }

    //-- Relative transform, spatial velocity, and spatial acceleration
    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialMotion(
              &desiredTf, relativeTo,
              &desiredVel, relativeTo, inCoordinatesOf,
              &desiredAcc, relativeTo, inCoordinatesOf);
        actualTf = bodyNode1->getTransform(relativeTo);
        actualVel = bodyNode1->getSpatialVelocity(
                      relativeTo, inCoordinatesOf);
        actualAcc = bodyNode1->getSpatialAcceleration(
                      relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(desiredTf.matrix(), actualTf.matrix()));
        EXPECT_TRUE(equals(desiredVel, actualVel));
        EXPECT_TRUE(equals(desiredAcc, actualAcc));
      }
    }


    //-- Relative classic linear acceleration

    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialAcceleration(
              desiredAcc, relativeTo, inCoordinatesOf);
        oldAngAcc
            = bodyNode1->getAngularAcceleration(relativeTo, inCoordinatesOf);
        joint1->setLinearAcceleration(
              desiredLinAcc, relativeTo, inCoordinatesOf);

        actualLinAcc
            = bodyNode1->getLinearAcceleration(relativeTo, inCoordinatesOf);
        actualAngAcc
            = bodyNode1->getAngularAcceleration(relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(desiredLinAcc, actualLinAcc));
        EXPECT_TRUE(equals(oldAngAcc, actualAngAcc));
      }
    }

    //-- Relative classic angular acceleration

    for (auto relativeTo : relFrames)
    {
      for (auto inCoordinatesOf : refFrames)
      {
        joint1->setSpatialAcceleration(
              desiredAcc, relativeTo, inCoordinatesOf);
        oldLinAcc
            = bodyNode1->getLinearAcceleration(relativeTo, inCoordinatesOf);
        joint1->setAngularAcceleration(
              desiredAngAcc, relativeTo, inCoordinatesOf);

        actualLinAcc =
            bodyNode1->getLinearAcceleration(relativeTo, inCoordinatesOf);
        actualAngAcc =
            bodyNode1->getAngularAcceleration(relativeTo, inCoordinatesOf);

        EXPECT_TRUE(equals(oldLinAcc, actualLinAcc));
        EXPECT_TRUE(equals(desiredAngAcc, actualAngAcc));
      }
    }
  }
}

//==============================================================================
int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

