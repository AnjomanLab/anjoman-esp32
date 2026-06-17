#include <Arduino.h>
#include <unity.h>

// Mock of Odometry Equations for testing verification
struct Pose {
    float x;
    float y;
    float theta;
};

Pose updateOdometry(float dLeftRad, float dRightRad, float wheelRadius, float wheelBase, Pose currentPose) {
    float dLeft = dLeftRad * wheelRadius;
    float dRight = dRightRad * wheelRadius;
    float dCenter = (dLeft + dRight) / 2.0f;
    float dTheta = (dRight - dLeft) / wheelBase;

    currentPose.x += dCenter * cos(currentPose.theta + dTheta / 2.0f);
    currentPose.y += dCenter * sin(currentPose.theta + dTheta / 2.0f);
    currentPose.theta += dTheta;

    // Normalizing theta to keep values bounded within [-PI, PI]
    while (currentPose.theta > PI) currentPose.theta -= 2.0f * PI;
    while (currentPose.theta < -PI) currentPose.theta += 2.0f * PI;

    return currentPose;
}

void test_straight_line_kinematics(void) {
    Pose initialPose = {0.0f, 0.0f, 0.0f};
    float wheelRadius = 0.0325f; // 32.5 mm in meters
    float wheelBase = 0.145f;    // 145 mm in meters

    // Simulate 10 full forward wheel rotations (2*PI * 10 radians)
    float dLeftRad = 2.0f * PI * 10.0f;
    float dRightRad = 2.0f * PI * 10.0f;

    Pose nextPose = updateOdometry(dLeftRad, dRightRad, wheelRadius, wheelBase, initialPose);

    // Calculate exact analytical expected x: 10 * 2 * PI * 0.0325 = 2.042035 meters
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.042f, nextPose.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.000f, nextPose.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.000f, nextPose.theta);
}

void test_in_place_rotation_kinematics(void) {
    Pose initialPose = {0.0f, 0.0f, 0.0f};
    float wheelRadius = 0.0325f;
    float wheelBase = 0.145f;

    // Simulate opposite rotations (Right forward, Left backward)
    float dLeftRad = -5.0f;
    float dRightRad = 5.0f;

    Pose nextPose = updateOdometry(dLeftRad, dRightRad, wheelRadius, wheelBase, initialPose);

    // dTheta expected = ( (5.0*0.0325) - (-5.0*0.0325) ) / 0.145 = 0.325 / 0.145 = 2.241379 rad
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.241f, nextPose.theta);
    TEST_ASSERT_FLOAT_WITHIN(0.010f, 0.000f, nextPose.x);
    TEST_ASSERT_FLOAT_WITHIN(0.010f, 0.000f, nextPose.y);
}

void setup() {
    delay(2000); // Guard time for UART connection stabilization
    UNITY_BEGIN();
    RUN_TEST(test_straight_line_kinematics);
    RUN_TEST(test_in_place_rotation_kinematics);
    UNITY_END();
}

void loop() {
    // Empty loop for unit tests
}
