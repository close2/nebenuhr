#define F_CPU 1000000UL

#include <avr/sleep.h>
#include "macros.h"
#include "tasks.h"
#include "clock.h"
#include "irqs.h"
#include "adc.h"


// wait timeSignalWaitPeriod ms until timeSignal is no longer present
// before turning IRQ for timeSignal on again.
const uint16_t timeSignalWaitPeriod = 10;

// when displayed time is not time, we move the minute finger faster
// but we will wait this pause (in ms) between every finger movement.
const uint16_t pauseBetweenHBridgePulses = 500;


template <typename T>
constexpr uint8_t voltsToUnits(T volts) {
  return 256 * volts / 1.1;
}

// 256 * minBatt1Voltage / 1.1 (1.1 ref voltage)
const auto batt1Min = voltsToUnits(0.8); // should be 186 units
const auto batt2Min = voltsToUnits(0.8); // should be 186 units
const auto batt3Min = voltsToUnits(0.8); // should be 186 units


typedef PIN_DIP_3 PauseClockPin;

// DIP_4 is INT1 irq
typedef PIN_DIP_4 CheckBatteryVoltagePin;

// DIP_5 is INT0 irq
typedef PIN_DIP_5 TimeSignalPin;

typedef PIN_DIP_6 NoonSensorPin;


typedef PIN_DIP_13 HBridge1;
typedef PIN_DIP_14 HBridge2;

typedef PIN_DIP_11 VoltageFetPin;
typedef PIN_DIP_28 Batt1Pin;
typedef PIN_DIP_27 Batt1OutPin;
typedef PIN_DIP_26 Batt2Pin;
typedef PIN_DIP_25 Batt2OutPin;
typedef PIN_DIP_24 Batt3Pin;
typedef PIN_DIP_23 Batt3OutPin;


// duration in units for hBridge output
uint16_t hBridgeOutDuration_units;

// using jumpers on the following pins let the user switch between different
// timings
typedef PIN_DIP_15 DurationIn0;
typedef PIN_DIP_16 DurationIn1;
typedef PIN_DIP_17 DurationIn2;
typedef PIN_DIP_18 DurationIn3;
typedef PIN_DIP_19 DurationIn4;

const uint16_t durations[32] = {
  ms_to_units((uint16_t) 130), // 0
  ms_to_units((uint16_t) 140),
  ms_to_units((uint16_t) 150),
  ms_to_units((uint16_t) 160),
  ms_to_units((uint16_t) 170),
  ms_to_units((uint16_t) 180),
  ms_to_units((uint16_t) 190),
  ms_to_units((uint16_t) 200),
  ms_to_units((uint16_t) 210),
  ms_to_units((uint16_t) 220),
  ms_to_units((uint16_t) 230), // 10
  ms_to_units((uint16_t) 240),
  ms_to_units((uint16_t) 250),
  ms_to_units((uint16_t) 260),
  ms_to_units((uint16_t) 270),
  ms_to_units((uint16_t) 280),
  ms_to_units((uint16_t) 290),
  ms_to_units((uint16_t) 300),
  ms_to_units((uint16_t) 320),
  ms_to_units((uint16_t) 340),
  ms_to_units((uint16_t) 360), // 20
  ms_to_units((uint16_t) 380),
  ms_to_units((uint16_t) 400),
  ms_to_units((uint16_t) 420),
  ms_to_units((uint16_t) 440),
  ms_to_units((uint16_t) 460),
  ms_to_units((uint16_t) 480),
  ms_to_units((uint16_t) 500),
  ms_to_units((uint16_t) 525),
  ms_to_units((uint16_t) 550),
  ms_to_units((uint16_t) 600), // 30
  ms_to_units((uint16_t) 650)
};

uint16_t time; // in minutes from 12:00; wraps every 12*60 minutes

// we allow the displayed time to be pause (for instance during the night,
// because the clock is really loud)
uint16_t displayedTime; // in minutes from 12:00; wraps every 12*60 minutes


///////////////////////////////  Task managment  ///////////////////////////////

enum Task {
  IncMinute = 0,
  CheckBatteries = 1,
  ReenableTimerSignalIrq = 2
};

uint8_t tasksToRun;


void startTask(Task task) {
  tasksToRun |= _BV(task);
}

void stopTask(Task task) {
  tasksToRun &= ~(_BV(task));
}

void stopAllTasks() {
  tasksToRun = 0;
}
  
uint8_t isAllTasksStopped() {
  return tasksToRun == 0;
}

uint8_t isTaskStarted(Task task) {
  return tasksToRun & ~(_BV(task));
}
////////////////////////////////////////////////////////////////////////////////


////////////////////  NoonSensor -- related functionality  /////////////////////

void setNoonSensor(uint8_t v) {
  SET_BIT(NoonSensorPin, PORT, v);
}

void initNoonSensor() {
  SET_BIT(NoonSensorPin, DDR, 1);
  
  // start with noonSensor "on"
  setNoonSensor(1);
}
////////////////////////////////////////////////////////////////////////////////


/////////////////  TimerSignal -- INT1 related functionality  //////////////////

void enableTimerSignalIRQ() {
  EIMSK |= _BV(INT1);
}

void disableTimerSignalIRQ() {
  EIMSK &= ~(_BV(INT1));
}

void initTimerSignal() {
  // nothing to do for pin
  // pins are Hi-Z input by default
  
  // turn on irq
  enableTimerSignalIRQ();
  sei();
}

#define NEW_IRQ_TASK IncMinuteIrqTask
IRQ_TASK(NEW_IRQ_TASK) {
  time++;
  if (time == 12 * 60) {
    time = 0;
    setNoonSensor(1);
  }
  else setNoonSensor(0);
        

  startTask(Task::IncMinute);
  startTask(Task::ReenableTimerSignalIrq);
  
  // turn off IRQ until signal from real clock is no longer present
  // otherwise this IRQ will trigger too often.
  disableTimerSignalIRQ();
};
#include "internal/register_irq_task_INT1.h"

uint8_t timerSignalPresent() {
  return GET_BIT(TimeSignalPin, PIN) == 0;
}

#define NEW_TASK ReenableTimerSignalIrqTask
struct NEW_TASK {
  static inline uint8_t is_enabled() {
    return isTaskStarted(Task::ReenableTimerSignalIrq);
  }
  
  template<typename T>
  static inline T run(const T& clock) {
    if (!timerSignalPresent()) {
      stopTask(Task::ReenableTimerSignalIrq);
      enableTimerSignalIRQ();
    }
    return ms_to_units(5);
  }
};
#include REGISTER_TASK
////////////////////////////////////////////////////////////////////////////////


/////////////////  CheckBattery -- INT0 related functionality  /////////////////

void enableCheckBatteryIRQ() {
  EIMSK |= _BV(INT0);
}

void disableCheckBatteryIRQ() {
  EIMSK &= ~(_BV(INT0));
}

void initCheckBatteryPin() {
  // nothing to do for input pin
  // pins are Hi-Z input by default
  
  // turn output pins to output:
  SET_BIT(Batt1OutPin, DDR, 1);
  SET_BIT(Batt2OutPin, DDR, 1);
  SET_BIT(Batt3OutPin, DDR, 1);
  
  // turn on irq
  enableCheckBatteryIRQ();
  sei();
}

#define NEW_IRQ_TASK CheckBatteryIrqTask
IRQ_TASK(NEW_IRQ_TASK) {
  startTask(Task::CheckBatteries);
  
  // turn off IRQ until button is no longer pressed
  // otherwise this IRQ will trigger too often.
  disableCheckBatteryIRQ();
};
#include "internal/register_irq_task_INT0.h"

uint8_t checkBatteryPressed() {
  return GET_BIT(CheckBatteryVoltagePin, PIN) == 0;
}
////////////////////////////////////////////////////////////////////////////////


////////////////////  PauseClock -- related functionality  /////////////////////

void initClockPaused() {
  // nothing to do
  // pins are Hi-Z input by default
}

uint8_t clockPaused() {
  return GET_BIT(PauseClockPin, PIN);
}
////////////////////////////////////////////////////////////////////////////////



///////////////  HBrdigeOutDuration -- related functionality  //////////////////

void initHBridge() {
  SET_BIT(HBridge1, DDR, 1);
  SET_BIT(HBridge2, DDR, 1);
}

void initHBridgeOutDuration_units() {
  uint8_t index;
  // pins are by default input
  index = GET8_BYTE(PIN, 0, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, DurationIn4, DurationIn3, DurationIn2, DurationIn1, DurationIn0);
  
  hBridgeOutDuration_units = durations[index];
}
////////////////////////////////////////////////////////////////////////////////


#define NEW_TASK IncMinuteTask
struct NEW_TASK {
  static inline uint8_t is_enabled() {
    return isTaskStarted(Task::IncMinute);
  }
  
  template<typename T>
  static inline T run(const T& clock) {
    // this task will be called twice for every minute increment.
    // first to turn the output on and second to turn it off again.
    
    // we also need to keep track if we have to send a positive or negative
    // signal.
    
    enum HBridgeState {
      On = 0,
      Off = 1
    };
    
    static HBridgeState hBridgeState = Off;
    
    const uint8_t clockP = clockPaused();
    
    const uint8_t bridgeSelector = displayedTime & 0b1;
    
    if (hBridgeState == Off) {
      if (!clockP) {
        if (bridgeSelector == 0) SET_BIT(HBridge1, PORT, 1);
        else SET_BIT(HBridge2, PORT, 1);

        displayedTime++;
        if (displayedTime == 12 * 60) displayedTime = 0;
      }
      
      hBridgeState = On;
      // sleep for hbridgeOutDuration
      return hBridgeOutDuration_units;
    }
    
    // state is on → turn off
    SET_BIT(HBridge1, PORT, 0);
    SET_BIT(HBridge2, PORT, 0);
      
    // we have to make sure the timeSignal is no longer present, before
    // turning irq for timeSignal on again.
    if (timerSignalPresent()) return ms_to_units(timeSignalWaitPeriod);
      
    // only then switch to next status:
    hBridgeState = Off;
    
    // mark IncMinuteTask as done:
    //             disable task in tasktracker
    stopTask(Task::IncMinute);
    
    if (!clockP && displayedTime != time) {
      // reenable incMinuteTask, need to correct displayed time
      startTask(Task::IncMinute);
      return ms_to_units(pauseBetweenHBridgePulses);
    }
    
    return 0;
  }
};
#include REGISTER_TASK


#define NEW_TASK CheckBatteriesTask
struct NEW_TASK {
  static inline uint8_t is_enabled() {
    return isTaskStarted(Task::CheckBatteries);
  }
  
  typedef Adc<_adc::Mode::SingleConversion, _adc::Ref::V1_1, Batt1Pin> Adc_Batt1;
  typedef Adc<_adc::Mode::SingleConversion, _adc::Ref::V1_1, Batt2Pin> Adc_Batt2;
  typedef Adc<_adc::Mode::SingleConversion, _adc::Ref::V1_1, Batt3Pin> Adc_Batt3;
  
  template<typename T>
  static inline T run(const T& clock) {
    // "turn on" fet to pass voltages from batteries:
    SET_BIT(VoltageFetPin, PORT, 1);
    
    Adc_Batt1::init();
    uint8_t batt1 = Adc_Batt1::adc_8bit();
    // don't need to turn off adc
    
    Adc_Batt2::init();
    uint8_t batt2 = Adc_Batt2::adc_8bit();
    // don't need to turn off adc
    
    Adc_Batt3::init();
    uint8_t batt3 = Adc_Batt3::adc_8bit();
    Adc_Batt3::turn_off();
    
    SET_BIT(Batt1OutPin, PORT, (batt1 > batt1Min));
    SET_BIT(Batt2OutPin, PORT, (batt2 > batt2Min));
    SET_BIT(Batt3OutPin, PORT, (batt3 > batt3Min));
    
    if (checkBatteryPressed()) return ms_to_units(200);
    
    // user no longer presses battery check button
    
    SET_BIT(VoltageFetPin, PORT, 0);
    
    // turn off all outputs:
    SET_BIT(Batt1OutPin, PORT, 0);
    SET_BIT(Batt2OutPin, PORT, 0);
    SET_BIT(Batt3OutPin, PORT, 0);
    
    stopTask(Task::CheckBatteries);
    
    enableCheckBatteryIRQ();
    return 0;
  }
};
#include REGISTER_TASK

//#define TEST3

#ifdef TEST1
  // test led output
  __attribute__ ((OS_main)) int main(void) {
    
    SET_BIT(Batt1OutPin, DDR, 1);
    SET_BIT(Batt2OutPin, DDR, 1);
    SET_BIT(Batt3OutPin, DDR, 1);
    
    SET_BIT(Batt1OutPin, PORT, 1);
    SET_BIT(Batt2OutPin, PORT, 1);
    SET_BIT(Batt3OutPin, PORT, 1);
    for (;;);
    return 0;
  }
#  define USE_ONLY_DEFINED_IRQS
#  include REGISTER_IRQS

#elif defined TEST2

  // test power down
  __attribute__ ((OS_main)) int main(void) {
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    
    for (;;) {
      cli();
      sleep_enable();
      sei();
      sleep_enable();
      sleep_cpu();
      
      // visual indicator if sleep isn't entered:
      SET_BIT(Batt1OutPin, DDR, 1);
      SET_BIT(Batt1OutPin, PIN, 1);
    }
    return 0;
  }
#  define USE_ONLY_DEFINED_IRQS
#  include REGISTER_IRQS

#elif defined TEST3
#  include <util/delay.h>

  // test hbridge
  __attribute__ ((OS_main)) int main(void) {
    
    initHBridge();
    for (;;) {
      SET_BIT(HBridge1, PORT, 1);
      _delay_ms(2000);
      SET_BIT(HBridge1, PORT, 0);
      _delay_ms(2000);
      SET_BIT(HBridge2, PORT, 1);
      _delay_ms(2000);
      SET_BIT(HBridge2, PORT, 0);
      _delay_ms(2000);
    }
    return 0;
  }
#  define USE_ONLY_DEFINED_IRQS
#  include REGISTER_IRQS

#elif defined TEST4
  // test power output wenn hbridge is off
  __attribute__ ((OS_main)) int main(void) {
    
    initHBridge();
    SET_BIT(HBridge1, PORT, 0);
    SET_BIT(HBridge2, PORT, 0);
    for (;;);
    return 0;
  }
#  define USE_ONLY_DEFINED_IRQS
#  include REGISTER_IRQS

#elif defined TEST5

__attribute__ ((OS_main)) int main(void) {
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // start with all tasks stopped
  stopAllTasks();
  
  displayedTime = 0;
  time = 0x0FFF;
  tasksToRun |= Task::IncMinute;
  
  initNoonSensor();
  initClockPaused();
  initTimerSignal();
  initHBridgeOutDuration_units();
  initHBridge();
  
  for (;;) {
    execTasks<uint16_t, TASK_LIST>();
    
    // if all tasks are done
    // power down
    // go to sleep without race conditions...
    cli();
    if (isAllTasksStopped()) {
      sleep_enable();
      sei();
      sleep_cpu();
      sleep_disable();
    }
    sei();
  }
  return 0;
}

#define USE_ONLY_DEFINED_IRQS
#include REGISTER_IRQS

#elif defined TEST6
#else


__attribute__ ((OS_main)) int main(void) {
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // start with all tasks stopped
  stopAllTasks();
  
  displayedTime = 0;
  time = 0;
  
  initNoonSensor();
  initClockPaused();
  initTimerSignal();
  initHBridgeOutDuration_units();
  initHBridge();
  
  for (;;) {
    execTasks<uint16_t, TASK_LIST>();
    
    // if all tasks are done
    // power down
    // go to sleep without race conditions...
    cli();
    if (isAllTasksStopped()) {
      sleep_enable();
      sei();
      sleep_cpu();
      sleep_disable();
    }
    sei();
  }
  return 0;
}

#define USE_ONLY_DEFINED_IRQS
#include REGISTER_IRQS

#endif
