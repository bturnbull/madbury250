//
// Madbury 250th Anniversary Cake Lighting Controller
//

#define STEP_DURATION     50       // milliseconds
#define SHORT              5       // number of STEP_DURATIONs for a short animation state
#define LONG              20       // number of STEP_DURATIONs for a long animation state
#define RESTART            0       // end of state list marker

#define CONTROL_PIN       12       // press and hold (~2s) to advance to next animation
#define CONTROL_COUNT     20       // number of STEP_DURATIONs to register a animation change

#define LED_PIN           13
static int led_state = 0;

#define NUM_CHANNELS      12           // ---lower----  --middle--  --upper---
static uint8_t pins[NUM_CHANNELS]     = { 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7 };
static uint16_t pins_polarity = 0b0000111111110000;

typedef struct {
  uint16_t channels;
  uint16_t duration;  // maintain state for duration count of STEP_DURATION, 0 = Restart animation
} state_t;

typedef struct {
  state_t* current;
  state_t* states;
} animation_t;

static state_t fullon_states[] = {
  { 0b0000111111111111, SHORT },
  { 0b0000111111111111, RESTART }
};
static animation_t fullon = { fullon_states, fullon_states };

static state_t fulloff_states[] = {
  { 0b0000000000000000, SHORT },
  { 0b0000000000000000, RESTART }
};
static animation_t fulloff = { fulloff_states, fulloff_states };

static state_t grow_states[] = {
  { 0b0000000110000001, SHORT },
  { 0b0000001111000011, SHORT },
  { 0b0000011111100111, SHORT },
  { 0b0000111111111111, SHORT },
  { 0b0000000000000001, RESTART }
};
static animation_t grow = { grow_states, grow_states };

static state_t chaser_states[] = {
  { 0b0000001111000011, SHORT },
  { 0b0000011001100110, SHORT },
  { 0b0000110000111100, SHORT },
  { 0b0000100110011001, SHORT },
  { 0b0000001111000011, RESTART }
};
static animation_t chaser = { chaser_states, chaser_states };

static state_t twinkle_states[] = {
  { 0b0000100100100100, SHORT },
  { 0b0000100010100100, SHORT },
  { 0b0000110010100100, SHORT },
  { 0b0000010000100110, SHORT },
  { 0b0000010000100010, SHORT },
  { 0b0000011000100010, SHORT },
  { 0b0000011010100010, SHORT },
  { 0b0000010010000110, SHORT },
  { 0b0000010110000101, SHORT },
  { 0b0000010110010001, SHORT },
  { 0b0000110000011001, SHORT },
  { 0b0000101000011001, SHORT },
  { 0b0000001000011101, SHORT },
  { 0b0000101000110101, SHORT },
  { 0b0000100001100001, SHORT },
  { 0b0000100001001001, SHORT },
  { 0b0000100001001000, SHORT },
  { 0b0000100101101000, SHORT },
  { 0b0000100101101100, SHORT },
  { 0b0000000110010100, RESTART }
};
static animation_t twinkle = { twinkle_states, twinkle_states };

static state_t layers_states[] = {
  { 0b0000000000000000, LONG },
  { 0b0000000000001111, LONG },
  { 0b0000000011111111, LONG },
  { 0b0000111111111111, LONG },
  { 0b0000111111111111, LONG },
  { 0b0000111111111111, LONG },
  { 0b0000000011111111, LONG },
  { 0b0000000000001111, LONG },
  { 0b0000000000000000, LONG },
  { 0b0000000000000000, RESTART }
};
static animation_t layers = { layers_states, layers_states };

static state_t updown_states[] = {
  { 0b0000000000000000, SHORT },
  { 0b0000000000000001, SHORT },
  { 0b0000000000000011, SHORT },
  { 0b0000000000000111, SHORT },
  { 0b0000000000001111, SHORT },
  { 0b0000000010001111, SHORT },
  { 0b0000000011001111, SHORT },
  { 0b0000000011101111, SHORT },
  { 0b0000000011111111, SHORT },
  { 0b0000000111111111, SHORT },
  { 0b0000001111111111, SHORT },
  { 0b0000011111111111, SHORT },
  { 0b0000111111111111, LONG },
  { 0b0000011111111111, SHORT },
  { 0b0000001111111111, SHORT },
  { 0b0000000111111111, SHORT },
  { 0b0000000011111111, SHORT },
  { 0b0000000011101111, SHORT },
  { 0b0000000011001111, SHORT },
  { 0b0000000010001111, SHORT },
  { 0b0000000000001111, SHORT },
  { 0b0000000000000111, SHORT },
  { 0b0000000000000011, SHORT },
  { 0b0000000000000001, SHORT },
  { 0b0000000000000000, LONG },
  { 0b0000000000000000, RESTART }
};
static animation_t updown = { updown_states, updown_states };

static animation_t* animations[] = {
  &fullon,
  &twinkle,
  &chaser,
  &updown,
  &layers,
  NULL
};


static int index = 0;
static animation_t* animation = animations[index];

static int step_count = 0;
static int control_count = 0;

static void toggle_led() {
  if (led_state == 0) { led_state = 1; }
  else { led_state = 0; }

  digitalWrite(LED_PIN, led_state);
}

static void init_pins() {
  pinMode(CONTROL_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, 0);
  pinMode(LED_PIN, OUTPUT);

  for (int i = 0; i < NUM_CHANNELS ; ++i) {
    digitalWrite(pins[i], (pins_polarity >> i) & 1);
    pinMode(pins[i], OUTPUT);
  }
}

static void set_channels(state_t* state) {
  for (int i = 0; i < NUM_CHANNELS; ++i) {
    digitalWrite(pins[i], ((pins_polarity ^ state->channels) & (1 << i)) >> i);
  }
}

static void set_animation(animation_t* animation) {
  animation->current = animation->states;
  ::animation = animation;
}

static void set_next_animation() {
  ++index;
  if (animations[index] == NULL) { index = 0; }

  set_animation(animations[index]);
}

static void set_next_state() {
  ++(animation->current);
  if (animation->current->duration == RESTART) { animation->current = animation->states; }

  set_channels(animation->current);
}

void setup() {
  init_pins();

  // run a little lightshow to check each channel - don't react to control input
  for (int i = 0; ; ++i) {
    if (grow.states[i].duration == RESTART) { break; }

    set_channels(&(grow.states[i]));
    delay(STEP_DURATION * grow.states[i].duration);
  }
}

void loop() {
  // check for button press
  if (digitalRead(CONTROL_PIN) == LOW) {
    ++control_count;
    toggle_led();
  } else {
    if (control_count > CONTROL_COUNT) {
      set_next_animation();
      step_count = 0;
      set_next_state();
    }
    control_count = 0;
  }

  // advance animation state if current state completed
  ++step_count;
  if (step_count > animation->current->duration) {
    step_count = 0;
    set_next_state();
  }

  delay(STEP_DURATION);
}
