#include <ros/ros.h>
#include <urdf/model.h>
#include <tf_conversions/tf_kdl.h>
#include <moveit/kinematics_base/kinematics_base.h>

#include "mir_arm_kinematics_youbot/arm_analytical_inverse_kinematics.h"

// Need a floating point tolerance when checking joint limits, in case the joint starts at limit
const double LIMIT_TOLERANCE = .0000001;

namespace arm_analytical_inverse_kinematics_youbot
{

#include "arm_ikfast_kinematics_solver_youbot.cpp"

class ArmAnalyticalInverseKinematicsYoubotPlugin : public kinematics::KinematicsBase
{
  std::vector<std::string> joint_names_;
  std::vector<double> joint_min_vector_;
  std::vector<double> joint_max_vector_;
  std::vector<bool> joint_has_limits_vector_;
  std::vector<std::string> link_names_;
  size_t num_joints_;
  std::vector<int> free_params_;
  bool active_; // Internal variable that indicates whether solvers are configured and ready
  boost::shared_ptr<ArmAnalyticalInverseKinematics> ik_solver_;

  const std::vector<std::string>& getJointNames() const { return joint_names_; }
  const std::vector<std::string>& getLinkNames() const { return link_names_; }

public:

  /** @class
   *  @brief Interface for an kinematics plugin
   */
  ArmAnalyticalInverseKinematicsYoubotPlugin():active_(false){}

  /**
   * @brief Given a desired pose of the end-effector, compute the joint angles to reach it
   * @param ik_pose the desired pose of the link
   * @param ik_seed_state an initial guess solution for the inverse kinematics
   * @param solution the solution vector
   * @param error_code an error code that encodes the reason for failure or success
   * @return True if a valid solution was found, false otherwise
   */

  // Returns the first IK solution that is within joint limits, this is called by get_ik() service
  bool getPositionIK(const geometry_msgs::Pose &ik_pose,
                     const std::vector<double> &ik_seed_state,
                     std::vector<double> &solution,
                     moveit_msgs::MoveItErrorCodes &error_code,
                     const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

  /**
   * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
   * This particular method is intended for "searching" for a solutions by stepping through the redundancy
   * (or other numerical routines).
   * @param ik_pose the desired pose of the link
   * @param ik_seed_state an initial guess solution for the inverse kinematics
   * @return True if a valid solution was found, false otherwise
   */
  bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                        const std::vector<double> &ik_seed_state,
                        double timeout,
                        std::vector<double> &solution,
                        moveit_msgs::MoveItErrorCodes &error_code,
                        const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

  /**
   * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
   * This particular method is intended for "searching" for a solutions by stepping through the redundancy
   * (or other numerical routines).
   * @param ik_pose the desired pose of the link
   * @param ik_seed_state an initial guess solution for the inverse kinematics
   * @param the distance that the redundancy can be from the current position
   * @return True if a valid solution was found, false otherwise
   */
  bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                        const std::vector<double> &ik_seed_state,
                        double timeout,
                        const std::vector<double> &consistency_limits,
                        std::vector<double> &solution,
                        moveit_msgs::MoveItErrorCodes &error_code,
                        const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

  /**
   * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
   * This particular method is intended for "searching" for a solutions by stepping through the redundancy
   * (or other numerical routines).
   * @param ik_pose the desired pose of the link
   * @param ik_seed_state an initial guess solution for the inverse kinematics
   * @return True if a valid solution was found, false otherwise
   */
  bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                        const std::vector<double> &ik_seed_state,
                        double timeout,
                        std::vector<double> &solution,
                        const IKCallbackFn &solution_callback,
                        moveit_msgs::MoveItErrorCodes &error_code,
                        const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

  /**
   * @brief Given a desired pose of the end-effector, search for the joint angles required to reach it.
   * This particular method is intended for "searching" for a solutions by stepping through the redundancy
   * (or other numerical routines).  The consistency_limit specifies that only certain redundancy positions
   * around those specified in the seed state are admissible and need to be searched.
   * @param ik_pose the desired pose of the link
   * @param ik_seed_state an initial guess solution for the inverse kinematics
   * @param consistency_limit the distance that the redundancy can be from the current position
   * @return True if a valid solution was found, false otherwise
   */
  bool searchPositionIK(const geometry_msgs::Pose &ik_pose,
                        const std::vector<double> &ik_seed_state,
                        double timeout,
                        const std::vector<double> &consistency_limits,
                        std::vector<double> &solution,
                        const IKCallbackFn &solution_callback,
                        moveit_msgs::MoveItErrorCodes &error_code,
                        const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const;

  /**
   * @brief Given a set of joint angles and a set of links, compute their pose
   *
   * This FK routine is only used if 'use_plugin_fk' is set in the 'arm_kinematics_constraint_aware' node,
   * otherwise ROS TF is used to calculate the forward kinematics
   *
   * @param link_names A set of links for which FK needs to be computed
   * @param joint_angles The state for which FK is being computed
   * @param poses The resultant set of poses (in the frame returned by getBaseFrame())
   * @return True if a valid solution was found, false otherwise
   */
  bool getPositionFK(const std::vector<std::string> &link_names,
                     const std::vector<double> &joint_angles,
                     std::vector<geometry_msgs::Pose> &poses) const;

private:

  bool initialize(const std::string &robot_description,
                  const std::string& group_name,
                  const std::string& base_name,
                  const std::string& tip_name,
                  double search_discretization);

  /**
   * @brief Calls the IK solver
   * @return The number of solutions found
   */
  int solve(KDL::Frame &pose_frame, const std::vector<double> &vfree, IkSolutionList<IkReal> &solutions) const;

  /**
   * @brief Gets a specific solution from the set
   */
  void getSolution(const IkSolutionList<IkReal> &solutions, int i, std::vector<double>& solution) const;

  double harmonize(const std::vector<double> &ik_seed_state, std::vector<double> &solution) const;
  //void getOrderedSolutions(const std::vector<double> &ik_seed_state, std::vector<std::vector<double> >& solslist);
  void getClosestSolution(const IkSolutionList<IkReal> &solutions, const std::vector<double> &ik_seed_state, std::vector<double> &solution) const;
  void fillFreeParams(int count, int *array);
  bool getCount(int &count, const int &max_count, const int &min_count) const;

}; // end class

bool ArmAnalyticalInverseKinematicsYoubotPlugin::initialize(const std::string &robot_description,
                                        const std::string& group_name,
                                        const std::string& base_name,
                                        const std::string& tip_name,
                                        double search_discretization)
{
  ROS_ERROR("BASE NAME: %s", base_name.c_str());

  setValues(robot_description, group_name, base_name, tip_name, search_discretization);

  ros::NodeHandle node_handle("~/"+group_name);

  std::string robot;
  node_handle.param("robot",robot,std::string());

  
  fillFreeParams( GetNumFreeParameters(), GetFreeParameters() );
  num_joints_ = GetNumJoints();

  if(free_params_.size() > 1)
  {
    ROS_FATAL("Only one free joint paramter supported!");
    return false;
  }

  urdf::Model robot_model;
  std::string xml_string;

  std::string urdf_xml,full_urdf_xml;
  node_handle.param("urdf_xml",urdf_xml,robot_description);
  node_handle.searchParam(urdf_xml,full_urdf_xml);

  ROS_DEBUG_NAMED("ik","Reading xml file from parameter server");
  if (!node_handle.getParam(full_urdf_xml, xml_string))
  {
    ROS_FATAL_NAMED("ik","Could not load the xml from parameter server: %s", urdf_xml.c_str());
    return false;
  }

  node_handle.param(full_urdf_xml,xml_string,std::string());
  robot_model.initString(xml_string);

  ROS_DEBUG_STREAM_NAMED("ik","Reading joints and links from URDF");

  boost::shared_ptr<urdf::Link> link = boost::const_pointer_cast<urdf::Link>(robot_model.getLink(tip_frame_));
  while(link->name != base_frame_ && joint_names_.size() <= num_joints_)
  {
    ROS_DEBUG_NAMED("ik","Link %s",link->name.c_str());
    link_names_.push_back(link->name);
    boost::shared_ptr<urdf::Joint> joint = link->parent_joint;
    if(joint)
    {
      if (joint->type != urdf::Joint::UNKNOWN && joint->type != urdf::Joint::FIXED)
      {
        ROS_DEBUG_STREAM_NAMED("ik","Adding joint " << joint->name );

        joint_names_.push_back(joint->name);
        float lower, upper;
        int hasLimits;
        if ( joint->type != urdf::Joint::CONTINUOUS )
        {
          if(joint->safety)
          {
            lower = joint->safety->soft_lower_limit;
            upper = joint->safety->soft_upper_limit;
          } else {
            lower = joint->limits->lower;
            upper = joint->limits->upper;
          }
          hasLimits = 1;
        }
        else
        {
          lower = -M_PI; 
          upper = M_PI;
          hasLimits = 0;
        }
        if(hasLimits)
        {
          joint_has_limits_vector_.push_back(true);
          joint_min_vector_.push_back(lower);
          joint_max_vector_.push_back(upper);
        }
        else
        {
          joint_has_limits_vector_.push_back(false);
          joint_min_vector_.push_back(-M_PI);
          joint_max_vector_.push_back(M_PI);
        }
      }
    } else
    {
      ROS_WARN_NAMED("ik","no joint corresponding to %s",link->name.c_str());
    }
    link = link->getParent();
  }

  if(joint_names_.size() != num_joints_)
  {
    ROS_FATAL_STREAM_NAMED("ik","Joint numbers mismatch: URDF has " << joint_names_.size() << " and IK solver has " << num_joints_);
    return false;
  }

  std::reverse(link_names_.begin(),link_names_.end());
  std::reverse(joint_names_.begin(),joint_names_.end());
  std::reverse(joint_min_vector_.begin(),joint_min_vector_.end());
  std::reverse(joint_max_vector_.begin(),joint_max_vector_.end());
  std::reverse(joint_has_limits_vector_.begin(), joint_has_limits_vector_.end());

  for(size_t i=0; i <num_joints_; ++i)
    ROS_INFO_STREAM_NAMED("ik",joint_names_[i] << " " << joint_min_vector_[i] << " " << joint_max_vector_[i] << " " << joint_has_limits_vector_[i]);

  ik_solver_ = boost::shared_ptr<ArmAnalyticalInverseKinematics>(
      new ArmAnalyticalInverseKinematics(joint_min_vector_, joint_max_vector_));

  active_ = true;
  return true;
}

int ArmAnalyticalInverseKinematicsYoubotPlugin::solve(KDL::Frame &pose_frame, const std::vector<double> &vfree, IkSolutionList<IkReal> &solutions) const
{
  
  solutions.Clear();

  double trans[3];
  trans[0] = pose_frame.p[0];//-.18;
  trans[1] = pose_frame.p[1];
  trans[2] = pose_frame.p[2];

  KDL::Rotation mult;
  KDL::Vector direction;
 
  direction = pose_frame.M * KDL::Vector(0, 0, 1);
  ComputeIk(trans, direction.data, vfree.size() > 0 ? &vfree[0] : NULL, solutions);

  return solutions.GetNumSolutions();
}

void ArmAnalyticalInverseKinematicsYoubotPlugin::getSolution(const IkSolutionList<IkReal> &solutions, int i, std::vector<double>& solution) const
{
  solution.clear();
  solution.resize(num_joints_);

  
  const IkSolutionBase<IkReal>& sol = solutions.GetSolution(i);
  std::vector<IkReal> vsolfree( sol.GetFree().size() );
  sol.GetSolution(&solution[0],vsolfree.size()>0?&vsolfree[0]:NULL); // ToDo: here

  // std::cout << "solution " << i << ":" ;
  // for(int j=0;j<num_joints_; ++j)
  //   std::cout << " " << solution[j];
  // std::cout << std::endl;

  //ROS_ERROR("%f %d",solution[2],vsolfree.size());
}

double ArmAnalyticalInverseKinematicsYoubotPlugin::harmonize(const std::vector<double> &ik_seed_state, std::vector<double> &solution) const
{
  double dist_sqr = 0;
  std::vector<double> ss = ik_seed_state;
  for(size_t i=0; i< ik_seed_state.size(); ++i)
  {
    while(ss[i] > 2*M_PI) {
      ss[i] -= 2*M_PI;
    }
    while(ss[i] < 2*M_PI) {
      ss[i] += 2*M_PI;
    }
    while(solution[i] > 2*M_PI) {
      solution[i] -= 2*M_PI;
    }
    while(solution[i] < 2*M_PI) {
      solution[i] += 2*M_PI;
    }
    dist_sqr += fabs(ik_seed_state[i] - solution[i]); 
  }
  return dist_sqr;
}

// void ArmAnalyticalInverseKinematicsYoubotPlugin::getOrderedSolutions(const std::vector<double> &ik_seed_state,
//                                  std::vector<std::vector<double> >& solslist)
// {
//   std::vector<double>
//   double mindist = 0;
//   int minindex = -1;
//   std::vector<double> sol;
//   for(size_t i=0;i<solslist.size();++i){
//     getSolution(i,sol);
//     double dist = harmonize(ik_seed_state, sol);
//     //std::cout << "dist[" << i << "]= " << dist << std::endl;
//     if(minindex == -1 || dist<mindist){
//       minindex = i;
//       mindist = dist;
//     }
//   }
//   if(minindex >= 0){
//     getSolution(minindex,solution);
//     harmonize(ik_seed_state, solution);
//     index = minindex;
//   }
// }

void ArmAnalyticalInverseKinematicsYoubotPlugin::getClosestSolution(const IkSolutionList<IkReal> &solutions, const std::vector<double> &ik_seed_state, std::vector<double> &solution) const
{
  double mindist = DBL_MAX;
  int minindex = -1;
  std::vector<double> sol;

  
  for(size_t i=0; i < solutions.GetNumSolutions(); ++i)
  {
    getSolution(solutions, i,sol);
    double dist = harmonize(ik_seed_state, sol);
    ROS_INFO_STREAM_NAMED("ik","Dist " << i << " dist " << dist);
    //std::cout << "dist[" << i << "]= " << dist << std::endl;
    if(minindex == -1 || dist<mindist){
      minindex = i;
      mindist = dist;
    }
  }
  if(minindex >= 0){
    getSolution(solutions, minindex,solution);
    harmonize(ik_seed_state, solution);
  }
}

void ArmAnalyticalInverseKinematicsYoubotPlugin::fillFreeParams(int count, int *array)
{
  free_params_.clear();
  for(int i=0; i<count;++i) free_params_.push_back(array[i]);
}

bool ArmAnalyticalInverseKinematicsYoubotPlugin::getCount(int &count, const int &max_count, const int &min_count) const
{
  if(count > 0)
  {
    if(-count >= min_count)
    {
      count = -count;
      return true;
    }
    else if(count+1 <= max_count)
    {
      count = count+1;
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    if(1-count <= max_count)
    {
      count = 1-count;
      return true;
    }
    else if(count-1 >= min_count)
    {
      count = count -1;
      return true;
    }
    else
      return false;
  }
}

bool ArmAnalyticalInverseKinematicsYoubotPlugin::getPositionFK(const std::vector<std::string> &link_names,
                                           const std::vector<double> &joint_angles,
                                           std::vector<geometry_msgs::Pose> &poses) const
{
#ifndef IKTYPE_TRANSFORM_6D
  ROS_ERROR_NAMED("ik", "Can only compute FK for IKTYPE_TRANSFORM_6D!");
  return false;
#endif

  KDL::Frame p_out;
  if(link_names.size() == 0) {
    ROS_WARN_STREAM_NAMED("ik","Link names with nothing");
    return false;
  }

  if(link_names.size()!=1 || link_names[0]!=tip_frame_){
    ROS_ERROR_NAMED("ik","Can compute FK for %s only",tip_frame_.c_str());
    return false;
  }

  bool valid = true;

  IkReal eerot[9],eetrans[3];
  IkReal angles[joint_angles.size()];
  for (unsigned char i=0; i < joint_angles.size(); i++)
    angles[i] = joint_angles[i];

  
  ComputeFk(angles,eetrans,eerot);

  for(int i=0; i<3;++i)
    p_out.p.data[i] = eetrans[i];

  for(int i=0; i<9;++i)
    p_out.M.data[i] = eerot[i];

  poses.resize(1);
  tf::poseKDLToMsg(p_out,poses[0]);

  return valid;
}

bool ArmAnalyticalInverseKinematicsYoubotPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                           const std::vector<double> &ik_seed_state,
                                           double timeout,
                                           std::vector<double> &solution,
                                           moveit_msgs::MoveItErrorCodes &error_code,
                                           const kinematics::KinematicsQueryOptions &options) const
{
  const IKCallbackFn solution_callback = 0; 
  std::vector<double> consistency_limits;

  return searchPositionIK(ik_pose,
                          ik_seed_state,
                          timeout,
                          consistency_limits,
                          solution,
                          solution_callback,
                          error_code,
                          options);
}
    
bool ArmAnalyticalInverseKinematicsYoubotPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                           const std::vector<double> &ik_seed_state,
                                           double timeout,
                                           const std::vector<double> &consistency_limits,
                                           std::vector<double> &solution,
                                           moveit_msgs::MoveItErrorCodes &error_code,
                                           const kinematics::KinematicsQueryOptions &options) const
{
  const IKCallbackFn solution_callback = 0; 
  return searchPositionIK(ik_pose,
                          ik_seed_state,
                          timeout,
                          consistency_limits,
                          solution,
                          solution_callback,
                          error_code,
                          options);
}

bool ArmAnalyticalInverseKinematicsYoubotPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                           const std::vector<double> &ik_seed_state,
                                           double timeout,
                                           std::vector<double> &solution,
                                           const IKCallbackFn &solution_callback,
                                           moveit_msgs::MoveItErrorCodes &error_code,
                                           const kinematics::KinematicsQueryOptions &options) const
{
  std::vector<double> consistency_limits;
  return searchPositionIK(ik_pose,
                          ik_seed_state,
                          timeout,
                          consistency_limits,
                          solution,
                          solution_callback,
                          error_code,
                          options);
}

bool ArmAnalyticalInverseKinematicsYoubotPlugin::searchPositionIK(const geometry_msgs::Pose &ik_pose,
                                              const std::vector<double> &ik_seed_state,
                                              double timeout,
                                              const std::vector<double> &consistency_limits,
                                              std::vector<double> &solution,
                                              const IKCallbackFn &solution_callback,
                                              moveit_msgs::MoveItErrorCodes &error_code,
                                              const kinematics::KinematicsQueryOptions &options) const
{
  ROS_DEBUG_STREAM_NAMED("ik","searchPositionIK");

  // Check if there are no redundant joints
  if(free_params_.size()==0)
  {
    ROS_DEBUG_STREAM_NAMED("ik","No need to search since no free params/redundant joints");

    // Find first IK solution, within joint limits
    if(!getPositionIK(ik_pose, ik_seed_state, solution, error_code))
    {
      ROS_DEBUG_STREAM_NAMED("ik","No solution whatsoever");
      error_code.val = moveit_msgs::MoveItErrorCodes::NO_IK_SOLUTION;
      return false;
    }

    // check for collisions if a callback is provided
    if( !solution_callback.empty() )
    {
      solution_callback(ik_pose, solution, error_code);
      if(error_code.val == moveit_msgs::MoveItErrorCodes::SUCCESS)
      {
        ROS_DEBUG_STREAM_NAMED("ik","Solution passes callback");
        return true;
      }
      else
      {
        ROS_DEBUG_STREAM_NAMED("ik","Solution has error code " << error_code);
        return false;
      }
    }
    else
    {
      return true; // no collision check callback provided
    }
  }

  // -------------------------------------------------------------------------------------------------
  // Error Checking
  if(!active_)
  {
    ROS_ERROR_STREAM_NAMED("ik","Kinematics not active");
    error_code.val = error_code.NO_IK_SOLUTION;
    return false;
  }

  if(ik_seed_state.size() != num_joints_)
  {
    ROS_ERROR_STREAM_NAMED("ik","Seed state must have size " << num_joints_ << " instead of size " << ik_seed_state.size());
    error_code.val = error_code.NO_IK_SOLUTION;
    return false;
  }

  if(!consistency_limits.empty() && consistency_limits.size() != num_joints_)
  {
    ROS_ERROR_STREAM_NAMED("ik","Consistency limits be empty or must have size " << num_joints_ << " instead of size " << consistency_limits.size());
    error_code.val = error_code.NO_IK_SOLUTION;
    return false;
  }


  // -------------------------------------------------------------------------------------------------
  // Initialize

  KDL::Frame frame;
  tf::poseMsgToKDL(ik_pose,frame);

  std::vector<double> vfree(free_params_.size());

  ros::Time maxTime = ros::Time::now() + ros::Duration(timeout);
  int counter = 0;

  double initial_guess = ik_seed_state[free_params_[0]];
  vfree[0] = initial_guess;

  // -------------------------------------------------------------------------------------------------
  // Handle consitency limits if needed
  int num_positive_increments;
  int num_negative_increments;

  if(!consistency_limits.empty())
  {
    // moveit replaced consistency_limit (scalar) w/ consistency_limits (vector)
    // Assume [0]th free_params element for now.  Probably wrong.
    double max_limit = fmin(joint_max_vector_[free_params_[0]], initial_guess+consistency_limits[free_params_[0]]);
    double min_limit = fmax(joint_min_vector_[free_params_[0]], initial_guess-consistency_limits[free_params_[0]]);

    num_positive_increments = (int)((max_limit-initial_guess)/search_discretization_);
    num_negative_increments = (int)((initial_guess-min_limit)/search_discretization_);
  }
  else // no consitency limits provided
  {
    num_positive_increments = (joint_max_vector_[free_params_[0]]-initial_guess)/search_discretization_;
    num_negative_increments = (initial_guess-joint_min_vector_[free_params_[0]])/search_discretization_;
  }

  // -------------------------------------------------------------------------------------------------
  // Begin searching

  ROS_DEBUG_STREAM_NAMED("ik","Free param is " << free_params_[0] << " initial guess is " << initial_guess << ", # positive increments: " << num_positive_increments << ", # negative increments: " << num_negative_increments);

  while(true)
  {
    IkSolutionList<IkReal> solutions;
    int numsol = solve(frame,vfree, solutions);

    ROS_DEBUG_STREAM_NAMED("ik","Found " << numsol << " solutions from IK");

    //ROS_INFO("%f",vfree[0]);

    if( numsol > 0 )
    {
      for(int s = 0; s < numsol; ++s)
      {
        std::vector<double> sol;
        getSolution(solutions,s,sol);

        bool obeys_limits = true;
        for(unsigned int i = 0; i < sol.size(); i++)
        {
          if(joint_has_limits_vector_[i] && (sol[i] < joint_min_vector_[i] || sol[i] > joint_max_vector_[i]))
          {
            obeys_limits = false;
            break;
          }
          //ROS_INFO_STREAM_NAMED("ik","Num " << i << " value " << sol[i] << " has limits " << joint_has_limits_vector_[i] << " " << joint_min_vector_[i] << " " << joint_max_vector_[i]);
        }
        if(obeys_limits)
        {
          getSolution(solutions,s,solution);

          // This solution is within joint limits, now check if in collision (if callback provided)
          if(!solution_callback.empty())
          {
            solution_callback(ik_pose, solution, error_code);
          }
          else
          {
            error_code.val = error_code.SUCCESS;
          }

          if(error_code.val == error_code.SUCCESS)
          {
            return true;
          }
        }
      }
    }

    if(!getCount(counter, num_positive_increments, num_negative_increments))
    {
      error_code.val = moveit_msgs::MoveItErrorCodes::NO_IK_SOLUTION;
      return false;
    }

    vfree[0] = initial_guess+search_discretization_*counter;
    ROS_DEBUG_STREAM_NAMED("ik","Attempt " << counter << " with 0th free joint having value " << vfree[0]);
  }

  // not really needed b/c shouldn't ever get here
  error_code.val = moveit_msgs::MoveItErrorCodes::NO_IK_SOLUTION;
  return false;
}

// Used when there are no redundant joints - aka no free params
bool ArmAnalyticalInverseKinematicsYoubotPlugin::getPositionIK(const geometry_msgs::Pose &ik_pose,
                                           const std::vector<double> &ik_seed_state,
                                           std::vector<double> &solution,
                                           moveit_msgs::MoveItErrorCodes &error_code,
                                           const kinematics::KinematicsQueryOptions &options) const
{
  ROS_DEBUG_STREAM_NAMED("ik","getPositionIK");

  if(!active_)
  {
    ROS_ERROR("kinematics not active");    
    return false;
  }

  KDL::Frame frame;
  tf::poseMsgToKDL(ik_pose,frame);


  std::size_t seed_size = ik_seed_state.size();
  KDL::JntArray seed(seed_size);
  for (std::size_t i = 0; i < seed_size; i++) seed(i) = ik_seed_state[i];

  std::vector<KDL::JntArray> solutions;
  ik_solver_->CartToJnt(seed, frame, solutions);
  int numsol = solutions.size();


  ROS_DEBUG_STREAM_NAMED("ik","Found " << numsol << " solutions from IK");

  if(numsol)
  {
    for(int s = 0; s < numsol; ++s)
    {
      std::vector<double> sol(solutions[s].rows());
      for (int j = 0; j < solutions[s].rows(); j++) sol[j] = solutions[s](j);
      ROS_DEBUG_NAMED("ik","Sol %d: %e   %e   %e   %e   %e   %e", s, sol[0], sol[1], sol[2], sol[3], sol[4], sol[5]);

      bool obeys_limits = true;
      for(unsigned int i = 0; i < sol.size(); i++)
      {
        // Add tolerance to limit check
        if(joint_has_limits_vector_[i] && ( (sol[i] < (joint_min_vector_[i]-LIMIT_TOLERANCE)) ||
                                            (sol[i] > (joint_max_vector_[i]+LIMIT_TOLERANCE)) ) )
        {
          // One element of solution is not within limits
          obeys_limits = false;
          ROS_DEBUG_STREAM_NAMED("ik","Not in limits! " << i << " value " << sol[i] << " has limit: " << joint_has_limits_vector_[i] << "  being  " << joint_min_vector_[i] << " to " << joint_max_vector_[i]);
          break;
        }
      }
      if(obeys_limits)
      {
        // All elements of solution obey limits
        solution.resize(solutions[s].rows());
        for (int j = 0; j < solutions[s].rows(); j++) solution[j] = solutions[s](j);
        error_code.val = moveit_msgs::MoveItErrorCodes::SUCCESS;
        return true;
      }
    }
  }
  else
  {
    ROS_DEBUG_STREAM_NAMED("ik","No IK solution");
  }

  error_code.val = moveit_msgs::MoveItErrorCodes::NO_IK_SOLUTION;
  return false;
}



} // end namespace

//register ArmAnalyticalInverseKinematicsYoubotPlugin as a KinematicsBase implementation
#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(arm_analytical_inverse_kinematics_youbot::ArmAnalyticalInverseKinematicsYoubotPlugin, kinematics::KinematicsBase);
