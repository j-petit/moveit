/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2019, Jens Petit
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Jens Petit */

#include <moveit/collision_detection_bullet/collision_robot_bt.h>
#include <moveit/collision_detection_bullet/tesseract/ros_tesseract_utils.h>

namespace collision_detection
{
CollisionRobotBt::CollisionRobotBt(const robot_model::RobotModelConstPtr& model, double padding, double scale)
  : CollisionRobot(model, padding, scale)
{
  auto fun = std::bind(&collision_detection::CollisionRobotBt::allowedCollisionCheck, this, std::placeholders::_1, std::placeholders::_2);

  bt_manager_.setIsContactAllowedFn(fun);
  bt_manager_CCD_.setIsContactAllowedFn(fun);

  for (const auto link : robot_model_->getURDF()->links_)
  {
    if (link.second->collision_array.size() > 0)
    {
      const std::vector<urdf::CollisionSharedPtr>& col_array =
          link.second->collision_array.empty() ? std::vector<urdf::CollisionSharedPtr>(1, link.second->collision) :
                                                 link.second->collision_array;

      std::vector<shapes::ShapeConstPtr> shapes;
      tesseract::VectorIsometry3d shape_poses;
      tesseract::CollisionObjectTypeVector collision_object_types;

      for (std::size_t i = 0; i < col_array.size(); ++i)
      {
        if (col_array[i] && col_array[i]->geometry)
        {
          shapes::ShapeConstPtr s = tesseract::tesseract_ros::constructShape(col_array[i]->geometry.get());
          if (s)
          {
            shapes.push_back(s);
            shape_poses.push_back(tesseract::tesseract_ros::urdfPose2Eigen(col_array[i]->origin));

            if (s->type == shapes::MESH)
            {
              collision_object_types.push_back(tesseract::CollisionObjectType::ConvexHull);
            }
            else
            {
              collision_object_types.push_back(tesseract::CollisionObjectType::UseShapeType);
            }
          }
        }
      }
      bt_manager_CCD_.addCollisionObject(link.second->name, 0, shapes, shape_poses, collision_object_types, true);
      bt_manager_.addCollisionObject(link.second->name, 0, shapes, shape_poses, collision_object_types, true);
    }
  }
  bt_manager_CCD_.setActiveCollisionObjects(robot_model_->getLinkModelNames());
}

CollisionRobotBt::CollisionRobotBt(const CollisionRobotBt& other) : CollisionRobot(other)
{
  const CollisionRobotBt& other_bt = dynamic_cast<const CollisionRobotBt&>(other);

  // TODO: use clone method of manager
  for (const auto& cow : other_bt.bt_manager_.getCollisionObjects())
  {
    tesseract::tesseract_bullet::COWPtr new_cow = cow.second->clone();
    new_cow->setWorldTransform(cow.second->getWorldTransform());
    new_cow->setContactProcessingThreshold(static_cast<btScalar>(other_bt.bt_manager_.getContactDistanceThreshold()));
    bt_manager_.addCollisionObject(new_cow);
  }

  bt_manager_.setActiveCollisionObjects(other_bt.bt_manager_.getActiveCollisionObjects());
  bt_manager_.setContactDistanceThreshold(other_bt.bt_manager_.getContactDistanceThreshold());
  bt_manager_.setIsContactAllowedFn(other_bt.bt_manager_.getIsContactAllowedFn());
}

void CollisionRobotBt::getAttachedBodyObjects(const robot_state::AttachedBody* ab, std::vector<void*>& geoms) const
{
  // TODO: Rewrite for using bullet add alll shapes to a single bullet COW. use geoms as output!
  const std::vector<shapes::ShapeConstPtr>& shapes = ab->getShapes();
  for (std::size_t i = 0; i < shapes.size(); ++i)
  {
  }
}

void CollisionRobotBt::addAttachedOjectsToManager(const robot_state::RobotState& state) const
{
  // TODO: add attached objects to the manager with correct flags using Bullet
  std::vector<const robot_state::AttachedBody*> ab;
  state.getAttachedBodies(ab);
  for (auto& body : ab)
  {
    std::vector<void*> objs;
    getAttachedBodyObjects(body, objs);
    const EigenSTL::vector_Isometry3d& ab_t = body->getGlobalCollisionBodyTransforms();
    for (std::size_t k = 0; k < objs.size(); ++k)
    {
    }
  }
}

void CollisionRobotBt::checkSelfCollision(const CollisionRequest& req, CollisionResult& res,
                                          const robot_state::RobotState& state) const
{
  checkSelfCollisionHelper(req, res, state, nullptr);
}

void CollisionRobotBt::checkSelfCollision(const CollisionRequest& req, CollisionResult& res,
                                          const robot_state::RobotState& state, const AllowedCollisionMatrix& acm) const
{
  checkSelfCollisionHelper(req, res, state, &acm);
}

void CollisionRobotBt::checkSelfCollision(const CollisionRequest& req, CollisionResult& res,
                                          const robot_state::RobotState& state1,
                                          const robot_state::RobotState& state2) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Bullet self continuous collision checking not yet implemented");
}

void CollisionRobotBt::checkSelfCollision(const CollisionRequest& req, CollisionResult& res,
                                          const robot_state::RobotState& state1, const robot_state::RobotState& state2,
                                          const AllowedCollisionMatrix& acm) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Bullet self continuous collision checking not yet implemented");
}

void CollisionRobotBt::checkSelfCollisionCCDHelper(const CollisionRequest& req, CollisionResult& res,
                                                   const robot_state::RobotState& state1,
                                                   const robot_state::RobotState& state2,
                                                   const AllowedCollisionMatrix* acm) const
{
  // TODO: Not in tesseract yet
  acm_ = acm;
  updateTransformsFromStateCCD(state1, state2);
  bt_manager_CCD_.contactTest(res, tesseract::ContactTestTypes::CLOSEST, req);
}

void CollisionRobotBt::checkSelfCollisionHelper(const CollisionRequest& req, CollisionResult& res,
                                                const robot_state::RobotState& state,
                                                const AllowedCollisionMatrix* acm) const
{
  acm_ = acm;

  updateTransformsFromState(state);
  bt_manager_.contactTest(res, tesseract::ContactTestType::FIRST, req);

  if (req.distance)
  {
    DistanceRequest dreq;
    DistanceResult dres;

    dreq.group_name = req.group_name;
    dreq.acm = acm;
    dreq.enableGroup(getRobotModel());
    distanceSelf(dreq, dres, state);
    res.distance = dres.minimum_distance.distance;
  }
}

void CollisionRobotBt::checkOtherCollision(const CollisionRequest& req, CollisionResult& res,
                                           const robot_state::RobotState& state, const CollisionRobot& other_robot,
                                           const robot_state::RobotState& other_state) const
{
  checkOtherCollisionHelper(req, res, state, other_robot, other_state, nullptr);
}

void CollisionRobotBt::checkOtherCollision(const CollisionRequest& req, CollisionResult& res,
                                           const robot_state::RobotState& state, const CollisionRobot& other_robot,
                                           const robot_state::RobotState& other_state,
                                           const AllowedCollisionMatrix& acm) const
{
  checkOtherCollisionHelper(req, res, state, other_robot, other_state, &acm);
}

void CollisionRobotBt::checkOtherCollision(const CollisionRequest& req, CollisionResult& res,
                                           const robot_state::RobotState& state1, const robot_state::RobotState& state2,
                                           const CollisionRobot& other_robot,
                                           const robot_state::RobotState& other_state1,
                                           const robot_state::RobotState& other_state2) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Bullet continuous collision checking not yet implemented");
}

void CollisionRobotBt::checkOtherCollision(const CollisionRequest& req, CollisionResult& res,
                                           const robot_state::RobotState& state1, const robot_state::RobotState& state2,
                                           const CollisionRobot& other_robot,
                                           const robot_state::RobotState& other_state1,
                                           const robot_state::RobotState& other_state2,
                                           const AllowedCollisionMatrix& acm) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Bullet continuous collision checking not yet implemented");
}

void CollisionRobotBt::checkOtherCollisionHelper(const CollisionRequest& req, CollisionResult& res,
                                                 const robot_state::RobotState& state,
                                                 const CollisionRobot& other_robot,
                                                 const robot_state::RobotState& other_state,
                                                 const AllowedCollisionMatrix* acm) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Other collision not implemented yet.");
}

void CollisionRobotBt::updatedPaddingOrScaling(const std::vector<std::string>& links)
{
  // TODO: add for bullet manager -> iterate through each input link and then construct new object
  // out of the geometry from the robot
  for (const auto& link : links)
  {
    const robot_model::LinkModel* lmodel = robot_model_->getLinkModel(link);
    if (lmodel)
    {
      for (std::size_t j = 0; j < lmodel->getShapes().size(); ++j)
      {
      }
    }
    else
      ROS_ERROR_NAMED("collision_detection.bullet", "Updating padding or scaling for unknown link: '%s'", link.c_str());
  }
}

void CollisionRobotBt::distanceSelf(const DistanceRequest& req, DistanceResult& res,
                                    const robot_state::RobotState& state) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Collision distance to self not implemented yet.");
}

void CollisionRobotBt::distanceOther(const DistanceRequest& req, DistanceResult& res,
                                     const robot_state::RobotState& state, const CollisionRobot& other_robot,
                                     const robot_state::RobotState& other_state) const
{
  ROS_ERROR_NAMED("collision_detection.bullet", "Collision distance to other not implemented yet.");
}

void CollisionRobotBt::updateTransformsFromState(const robot_state::RobotState& state) const
{
  // updating link positions with the current robot state
  for (auto& link : bt_manager_.getCollisionObjects())
  {
    // select the first of the transformations for each link (composed of multiple shapes...)
    // TODO: further investigate if this brings problems
    bt_manager_.setCollisionObjectsTransform(link.first, state.getCollisionBodyTransform(link.first, 0));
  }
}

void CollisionRobotBt::updateTransformsFromStateCCD(const robot_state::RobotState& state1, const robot_state::RobotState& state2) const
{
  // updating link positions with the current robot state
  for (auto& link : bt_manager_CCD_.getActiveCollisionObjects())
  {
    // select the first of the transformations for each link (composed of multiple shapes...)
    // TODO: further investigate if this brings problems
    bt_manager_CCD_.setCollisionObjectsTransform(link, state1.getCollisionBodyTransform(link, 0), state2.getCollisionBodyTransform(link, 0));
  }
}

bool CollisionRobotBt::allowedCollisionCheck(std::string body_1, std::string body_2)
{
  collision_detection::AllowedCollision::Type allowed_type;

  if (acm_ != nullptr)
  {
    if (acm_->getEntry(body_1, body_2, allowed_type))
    {
      if (allowed_type == collision_detection::AllowedCollision::Type::NEVER)
      {
        ROS_DEBUG_STREAM("Not allowed entry in ACM found, collision check between " << body_1 << " and " << body_2);
        return false;
      }
      else
      {
        ROS_DEBUG_STREAM("Entry in ACM found, skipping collision check as allowed " << body_1 << " and " << body_2);
        return true;
      }
    }
    else
    {
      ROS_DEBUG_STREAM("No entry in ACM found, collision check between " << body_1 << " and " << body_2);
      return false;
    }
  }
  else
  {
    ROS_DEBUG_STREAM("No ACM, collision check between " << body_1 << " and " << body_2);
    return false;
  }
}

}  // end of namespace collision_detection
