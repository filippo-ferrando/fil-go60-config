#define DT_DRV_COMPAT zmk_input_processor_gesture

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zmk/behavior.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct gesture_config {
  int32_t x_threshold;
  int32_t y_threshold;
  int32_t cooldown_ms;
  struct zmk_behavior_binding swipe_left;
  struct zmk_behavior_binding swipe_right;
  struct zmk_behavior_binding swipe_up;
  struct zmk_behavior_binding swipe_down;
};

struct gesture_data {
  int32_t acc_x;
  int32_t acc_y;
  int64_t last_trigger;
};

static void invoke_behavior(const struct zmk_behavior_binding *binding) {
  if (!binding || !binding->behavior_dev)
    return;

  struct zmk_behavior_binding_event event = {.position = 0,
                                             .timestamp = k_uptime_get()};

  zmk_behavior_invoke_binding((struct zmk_behavior_binding *)binding, event,
                              true);
  zmk_behavior_invoke_binding((struct zmk_behavior_binding *)binding, event,
                              false);
}

static int gesture_process(const struct device *dev,
                           struct input_event *event) {
  const struct gesture_config *cfg = dev->config;
  struct gesture_data *data = dev->data;

  if (event->type != INPUT_EV_REL ||
      (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
    return 0;
  }

  int64_t now = k_uptime_get();

  if (now - data->last_trigger > cfg->cooldown_ms * 2) {
    data->acc_x = 0;
    data->acc_y = 0;
  }

  if (event->code == INPUT_REL_X)
    data->acc_x += event->value;
  if (event->code == INPUT_REL_Y)
    data->acc_y += event->value;

  event->value = 0;

  if (now - data->last_trigger < cfg->cooldown_ms) {
    return -EINVAL;
  }

  bool triggered = false;

  if (data->acc_x > cfg->x_threshold) {
    invoke_behavior(&cfg->swipe_right);
    triggered = true;
  } else if (data->acc_x < -cfg->x_threshold) {
    invoke_behavior(&cfg->swipe_left);
    triggered = true;
  } else if (data->acc_y > cfg->y_threshold) {
    invoke_behavior(&cfg->swipe_down);
    triggered = true;
  } else if (data->acc_y < -cfg->y_threshold) {
    invoke_behavior(&cfg->swipe_up);
    triggered = true;
  }

  if (triggered) {
    data->acc_x = 0;
    data->acc_y = 0;
    data->last_trigger = now;
  }

  return -EINVAL;
}

#define GESTURE_BEHAVIOR_INIT_IMPL(n, prop)                                    \
  {                                                                            \
      .behavior_dev =                                                          \
          DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(DT_DRV_INST(n), prop, 0)),          \
      .param1 =                                                                \
          COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(DT_DRV_INST(n), prop, 0, param1), \
                      (0), (DT_PHA_BY_IDX(DT_DRV_INST(n), prop, 0, param1))),  \
      .param2 =                                                                \
          COND_CODE_0(DT_PHA_HAS_CELL_AT_IDX(DT_DRV_INST(n), prop, 0, param2), \
                      (0), (DT_PHA_BY_IDX(DT_DRV_INST(n), prop, 0, param2))),  \
  }

#define GESTURE_BEHAVIOR_INIT_EMPTY()                                          \
  {.behavior_dev = NULL, .param1 = 0, .param2 = 0}

#define GESTURE_BEHAVIOR_INIT(n, prop)                                         \
  COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), prop),                          \
              (GESTURE_BEHAVIOR_INIT_IMPL(n, prop)),                           \
              (GESTURE_BEHAVIOR_INIT_EMPTY()))

#define GESTURE_INST(n)                                                        \
  static struct gesture_data gesture_data_##n = {                              \
      .acc_x = 0,                                                              \
      .acc_y = 0,                                                              \
      .last_trigger = 0,                                                       \
  };                                                                           \
  static const struct gesture_config gesture_config_##n = {                    \
      .x_threshold = DT_INST_PROP(n, x_threshold),                             \
      .y_threshold = DT_INST_PROP(n, y_threshold),                             \
      .cooldown_ms = DT_INST_PROP(n, cooldown_ms),                             \
      .swipe_left = GESTURE_BEHAVIOR_INIT(n, swipe_left_behaviors),            \
      .swipe_right = GESTURE_BEHAVIOR_INIT(n, swipe_right_behaviors),          \
      .swipe_up = GESTURE_BEHAVIOR_INIT(n, swipe_up_behaviors),                \
      .swipe_down = GESTURE_BEHAVIOR_INIT(n, swipe_down_behaviors),            \
  };                                                                           \
  DEVICE_DT_INST_DEFINE(n, NULL, NULL, &gesture_data_##n, &gesture_config_##n, \
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,      \
                        gesture_process);

DT_INST_FOREACH_STATUS_OKAY(GESTURE_INST)
