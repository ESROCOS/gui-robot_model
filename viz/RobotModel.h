#ifndef ROBOTMODEL_H
#define ROBOTMODEL_H

#include <urdf_model/model.h>
#include <urdf_world/types.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/chain.hpp>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgText/Text>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgFX/Outline>
#include <QObject>
#include <QDir>

/** 
 * Struct to hold mimic joint properties
 */
struct MimicJoint {
    std::string jointToMimic;
    double multiplier;
    double offset;

    MimicJoint( std::string jointToMimic_ = "",
            double multiplier_ = 1,
            double offset_ = 0 )
        : jointToMimic( jointToMimic_ ),
        multiplier( multiplier_ ),
        offset( offset_ ) { }
};

/**
 * @brief Data structure that is attached as 'User Data' to nodes within the RobotModel.
 *
 * This data structure provides access to the visualization joints and segments of the robot.
 * Joint posiiotns can beupdated, Segemetns can be highlighted and labels can be attached to
 * the robot visualization.
 * Hereby internally, the same instance of OSGSegment is shared between the corresponding segment-,
 * joint-, and label nodes.
 *
 * This class should only be constructed by RobotModel.
 */
class OSGSegment : public osg::Referenced
{
public:
/**
 * @brief This class should only be constructed by RobotModel.
 *
 * @param node: The Node this OSGSegment is attached to. Should be the joint transform
 * @param seg: KDL segment corresponding to the node.
 */
    OSGSegment(KDL::Segment seg);

    /**
     * @brief Set position of joint
     *
     * @param rad: Position of joint in radians
     */
    void setJointPos(double rad){jointPos_ = rad;}

    /**
     * @brief Remove curretn label from node
     *
     */
    void removeLabel();

    /**
     * @brief Attach a label to node
     *
     * @param name: A name of the label, currently not internally used.
     * @param filepath: Path to an image file for the label. Should 256x256 Pixel.
     */
    void attachLabel(std::string name, std::string filepath);


    void setupTextLabel();
    /**
     * Attach a text label to a segemnt. If en empty string is given, the segment name is used
     * as label.
     */
    void attachTextLabel(std::string text="");

    /**
     * Remove text label
     */
    void removeTextLabel();

    /**
     * @brief Switch on/off highlighting visualization
     *
     * @return bool: New state of highlight (on/off)
     */
    bool toggleSelected();

    /**
     * @brief Returns the corresponding KDL::Segment
     *
     * @return const KDL::Segment
     */
    const KDL::Segment& getSegment(){return seg_;}
    
    /**
     * @brief returns the osg Node that is the root of the visualization for
     *   this segment
     */
    osg::ref_ptr<osg::Group> getGroup() const; 

protected:
    /**
     * @brief Updates the joint transformation corresponding to the value of joint_pos_.
     *
     */
    void updateJoint();

    /**
    * @brief Attach visual mesh to node.
    *
    * Should only be called during initial construction of the robot model.
    *
    * @param visual: Parsed URDF tag (using urdf_parser) of the visual.
    */
   void attachVisual(urdf::VisualSharedPtr visual, QDir prefix = QDir());

   /**
   * @brieg Create nodes with visual meshes
   * 
   * Should only be called during initial construction of the robot model
   *
   * @param visual_array: Parsed URDF tag (using urdf_parser) of the visual.
   */
   void attachVisuals(std::vector<urdf::VisualSharedPtr > &visual_array, QDir prefix = QDir());

private:
    KDL::Segment seg_; /**< KDKL representation of the segment */
    KDL::Frame toTipKdl_; /**< Temp storage for the current joint pose */
    osg::ref_ptr<osg::PositionAttitudeTransform> toTipOsg_; /**< The osg node for the joint pose */
    osg::ref_ptr<osg::Group> post_transform_;
    float jointPos_; /**< Current joint position in radians */
    osg::ref_ptr<osg::Geode> visual_; /**< OSG node for visual element */
    osg::ref_ptr<osg::Geode> label_; /**< OSG node for label */
    osg::ref_ptr<osgText::Text> text_label_;
    osg::ref_ptr<osg::Geode> text_label_geode_;
    bool isSelected_; /**< Selection state */

    friend class InteractionHandler;
    friend class OSGSegmentCallback;
    friend class RobotModel;
    friend class URDFRobotWidget;
    friend class CameraFollowNodeCallback;
};


/**
 * @brief This callback for user defined calculations before rendering.
 *
 */
class OSGSegmentCallback : public osg::NodeCallback
{
public:
    /**
     * @brief the function that gets called.
     *
     * This callback is attached to every joint transformation node. It is
     * executed before rendering every frame. The OSG joint transformation
     * node gets updated by new transformation (depending on it joint_position_).
     *
     * @param node: The corresponding node this Callback is attached to.
     * @param nv: A node visitor (a thing that traverses throu nodes)
     */
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::ref_ptr<OSGSegment> segment =
                dynamic_cast<OSGSegment*> (node->getUserData() );
        if(segment)
        {
            segment->updateJoint();
        }
        traverse(node, nv);
    }
};

/**
 * @brief This class is used for creating a visualization of a robot model and access it.
 *
 */
class RobotModel
{
public:
/**
 * @brief Constructor, does nearly nothing.
 *
 */
    RobotModel();

    /**
     * @brief
     *
     */
    ~RobotModel(){}

    /**
     * @brief Create visualization of a robot described as urdf file.
     *
     * @param path: Path to URDF file.
     * @return osg::Node: Root node of the constructed OSG scene for the robot.
     */
    osg::ref_ptr<osg::Node> load(QString path);

    /**
     * @brief Create an empty scene.
     *
     * This function removes to an existing robot from the scene.
     *
     * @return osg::Node: Root to the empty scene.
     */
    osg::ref_ptr<osg::Node> loadEmptyScene();

    /**
     * @brief Retrieve an OSG Segment by Joint or Segment name.
     *
     * @param name: Name of joint or segment (from URDF file)
     * @return OSGSegment
     */
    osg::ref_ptr<OSGSegment> getSegment(std::string name);

    /**
     * @brief Retrieve joint names of all joints defined in URDF file which are not of type KDL::Joint::None.
     *
     * @return const std::vector<std::string>
     */
    const std::vector<std::string>& getJointNames(){return jointNames_;}

    /**
     * @brief Retrieve all segment names defined in URDF file
     *
     * @return const std::vector<std::string>
     */
    const std::vector<std::string>& getSegmentNames(){return segmentNames_;}

    /**
     * @brief Calculate relative transform between two segments
     *
     * Will return the transform required to transform a point described in coordinate frame of
     * 'source_segment' to the frame 'target_segment'.
     *
     * @return osg::Matrixd
     */
    osg::Matrixd getRelativeTransform(std::string source_segment, std::string target_segment);

    /**
     * @brief Relocate the root to a given segment
     */
    bool relocateRoot(std::string name);

    /** 
     * @brief Finds the relevant node and sets the position value
     */
    bool setJointPos(std::string jointName, double jointVal);

protected:
    osg::ref_ptr<osg::Node> makeOsg2(KDL::Segment kdl_seg, urdf::Link urdf_link, osg::ref_ptr<osg::Group> root);
    osg::ref_ptr<osg::Node> makeOsg( urdf::ModelInterfaceSharedPtr urdf_model );

protected:
    osg::ref_ptr<osg::Group> root_; /**< Root of the OSG scene containing the robot */
    osg::ref_ptr<osg::Group> original_root_; /**< The original root always corresponding to the root of the URDF. If relocateRoot was called this get's not affected */
    std::string current_root_name_;
    std::string original_root_name_;
    std::vector<std::string> jointNames_; /**< Joint names defined in URDF (joint of type none are NOT included) */
    std::vector<std::string> segmentNames_; /**< Segment names defined in URDF */
    QDir rootPrefix;

    std::map< std::string, MimicJoint > mimic_joints_;

public:
    //bool set_joint_state(std::vector<double> joint_vals);
    /**
     * @brief Set position of a single joint by its name.
     *
     * @param std::string: Name of the joint
     * @param joint_val: Joint position in degree
     * @return bool
     */
    bool setJointState(std::string, double jointVal);
    bool setJointState(const std::map<std::string,double>& jointVals);
    bool toggleHighlight(std::string);

    friend class InteractionHandler;
    friend class URDFRobotWidget;
};

#endif
