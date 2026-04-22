import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    # ── Find where packages are installed ─────────────────────────────
    pkg_desc    = get_package_share_directory('simple_bot_description')
    pkg_bringup = get_package_share_directory('simple_bot_bringup')
    pkg_gazebo  = get_package_share_directory('ros_gz_sim')

    # ── Declare arguments (overridable on command line) ────────────────
    x_pose_arg = DeclareLaunchArgument('x_pose', default_value='0.0')
    y_pose_arg = DeclareLaunchArgument('y_pose', default_value='0.0')
    x_pose = LaunchConfiguration('x_pose')
    y_pose = LaunchConfiguration('y_pose')

    # ── Read the URDF file ─────────────────────────────────────────────
    urdf_file = os.path.join(pkg_desc, 'urdf', 'simple_bot.urdf')
    with open(urdf_file, 'r') as f:
        robot_urdf = f.read()
    robot_description = ParameterValue(robot_urdf, value_type=str)

    # ── Node 1: robot_state_publisher ─────────────────────────────────
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description,
                     'use_sim_time': True}]
    )

    # ── Process 2: Gazebo ─────────────────────────────────────────────
    world_file = os.path.join(pkg_bringup, 'worlds', 'curved_line.sdf')
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_gazebo, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={
            'gz_args': f'-r {world_file}',   # -r = run immediately (not paused)
            'on_exit_shutdown': 'true',       # kill launch if Gazebo closes
        }.items(),
    )

    # ── Node 3: spawn robot (after 8 s, gives Gazebo time to start) ───
    spawn_robot = TimerAction(
        period=15.0,
        actions=[Node(
            package='ros_gz_sim',
            executable='create',
            arguments=['-name', 'simple_bot',
                       '-topic', 'robot_description',
                       '-x', x_pose, '-y', y_pose, '-z', '0.05'],
        )]
    )

    # ── Node 4: line follower (after 12 s) ────────────────────────────
    follower_node = TimerAction(
        period=20.0,
        actions=[Node(
            package='line_follower_py',
            executable='follower',
            name='line_follower_node',
            output='screen',
            parameters=[{'use_sim_time': True}],
        )]
    )

    return LaunchDescription([
        x_pose_arg, y_pose_arg,
        robot_state_publisher,
        gazebo,
        spawn_robot,
        follower_node,
    ])