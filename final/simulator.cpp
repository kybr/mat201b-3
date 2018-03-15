// Diarmid Flatley
// 2018-2-26
// MAT 201B
// tuning lattice

#include "common.hpp"

#include "Gamma/Envelope.h"
#include "Gamma/Filter.h"
#include "Gamma/Oscillator.h"
#include "allocore/io/al_App.hpp"

#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_Simulator.hpp"

using namespace std;
using namespace al;

Vec3f midpoint(Vec3f a, Vec3f b, Vec3f c) {
  Vec3f middle;
  middle.x = (a.x + b.x + c.x) / 3;
  middle.y = (a.y + b.y + c.y) / 3;
  middle.z = (a.z + b.z + c.z) / 3;
  return middle;
}

struct Node {
  Vec3f position = Vec3f(0, 0, 0);
  vector<int> connections;
  float frequency;

  Node() {}

  Node(Vec3f initPos, vector<int> initConnections, float initFreq) {
    position = initPos;
    connections = initConnections;
    frequency = initFreq;
  }

  void set(float x, float y, float z) { position = Vec3f(x, y, z); }

  void set(Vec3f setPos) { position = setPos; }

  void draw(Graphics& g, Mesh& m) {
    m.color(1, 1, 1);
    g.pushMatrix();
    g.translate(position);
    g.draw(m);
    g.popMatrix();
  }
};

struct Cursor {
  Vec3f position;
  float counter;
  float increment;
  float currentFrequency;
  Node start;
  Node end;
  bool trigger;

  Cursor() {
    increment = 0.1f;
    counter = 0.0f;
    currentFrequency = 0.0f;
  }

  void set(Node initStart, Node initEnd) {
    position = initStart.position;
    start = initStart;
    end = initEnd;
    currentFrequency = start.frequency;
  }

  void update(Node node[]) {
    Vec3f startPos = start.position;
    Vec3f endPos = end.position;
    position = startPos.lerp(endPos, counter);
    counter += increment;
    if (counter > 1) {
      //  counter -= 1;
      trigger = true;
      unsigned i = rand() % end.connections.size();
      int next = end.connections[i];
      this->set(end, node[next]);
      counter -= 1;
    }
    // position = startPos.lerp(endPos, counter);
  }

  void draw(Graphics& g, Mesh& m) {
    m.color(0, 0, 1);
    g.pushMatrix();
    g.translate(position);
    g.draw(m);
    g.popMatrix();
  }
};

struct Strut {
  Vec3f start, end;

  Strut() {}

  Strut(Node initStart, Node initEnd) {
    start = initStart.position;
    end = initEnd.position;
  }

  void set(Node setStart, Node setEnd) {
    start = setStart.position;
    end = setEnd.position;
  }

  void draw(Graphics& g, Mesh& m) {
    m.reset();
    m.color(1, 1, 1);
    m.primitive(Graphics::LINES);
    m.stroke(2);
    m.vertex(start);
    m.vertex(end);
    g.draw(m);
  }
};

// struct AlloApp : App {
struct AlloApp : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  Material material;
  Light light;
  Mesh sphere;
  Mesh line;
  Mesh sphere2;
  Mesh sphere3;

  Node node[14];
  Vec3f vertex[14];
  vector<Strut*> struts;
  vector<int> connections[14];
  float frequency[14];  // = {350, 250, 218.75, 375, 262.5, 300,
                        // 291.66666, 312.5, 328.125, 214.285715,
                        // 200, 306.25, 225, 210};

  float fundamental = 200.0f;

  float a = 1;
  float b = 1.12;
  float c = 1.37;
  float d = 1.53;

  Cursor cursor;
  // Cursor cursor2;

  gam::Sine<> sine[8];
  float timbreFrequency[8] = {2.3, 3.8, 5.2, 5.8, 0, 0, 0, 0};
  float timbreAmplitude[8] = {0.28, 0.23, 0.16, 0.1, 0, 0, 0, 0};
  gam::AD<> env;

  // gam::Sine<> sine2;

  State state;
  cuttlebone::Maker<State> maker;

  AlloApp()
      : maker(Simulator::defaultBroadcastIP()),
        InterfaceServerClient(Simulator::defaultInterfaceServerIP()) {
    nav().pos(0, 0, 20);
    light.pos(0, 0, 0);

    addSphere(sphere, 0.1);
    addSphere(sphere2, 0.1);
    // addSphere(sphere3, 0.1);
    sphere.generateNormals();
    sphere2.generateNormals();
    //  sphere3.generateNormals();

    vertex[0] = {-1, 0, 0};
    vertex[1] = {0, 0, -1};
    vertex[2] = {0, 1, 0};
    vertex[3] = {1, 0, 0};
    vertex[4] = {0, 0, 1};
    vertex[5] = {0, -1, 0};
    vertex[6] = {-1, 1, -1};
    vertex[7] = {1, 1, -1};
    vertex[8] = {1, 1, 1};
    vertex[9] = {-1, 1, 1};
    vertex[10] = {-1, -1, -1};
    vertex[11] = {1, -1, -1};
    vertex[12] = {1, -1, 1};
    vertex[13] = {-1, -1, 1};

    connections[0] = {1, 2, 4, 5, 6, 9, 10, 13};
    connections[1] = {0, 2, 3, 5, 6, 7, 10, 11};
    connections[2] = {0, 1, 3, 4, 6, 7, 8, 9};
    connections[3] = {1, 2, 4, 5, 7, 8, 11, 12};
    connections[4] = {0, 2, 3, 5, 8, 9, 12, 13};
    connections[5] = {0, 1, 3, 4, 10, 11, 12, 13};
    connections[6] = {0, 1, 2};
    connections[7] = {1, 2, 3};
    connections[8] = {2, 3, 4};
    connections[9] = {0, 2, 4};
    connections[10] = {0, 1, 5};
    connections[11] = {1, 3, 5};
    connections[12] = {3, 4, 5};
    connections[13] = {0, 4, 5};

    frequency[0] = a * d;
    frequency[1] = a * b;
    frequency[2] = b * d;
    frequency[3] = b * c;
    frequency[4] = c * d;
    frequency[5] = a * c;
    frequency[6] = (a * b * d) / c;
    frequency[7] = b * b;
    frequency[8] = (b * c * d) / a;
    frequency[9] = d * d;
    frequency[10] = a * a;
    frequency[11] = (a * b * c) / d;
    frequency[12] = c * c;
    frequency[13] = (a * c * d) / b;

    for (int i = 0; i < 14; i++) {
      node[i] = {vertex[i], connections[i], frequency[i]};
      //  state.vertex[i] = vertex[i];
      //  state.connections[i] = connections[i];
    }

    int strutCount = 0;
    for (int i = 0; i < 14; i++) {
      for (int j = 0; j < node[i].connections.size(); j++) {
        Strut* strut = new Strut;
        struts.push_back(strut);
        struts[strutCount]->set(node[i], node[node[i].connections[j]]);
        strutCount++;
      }
    }

    cursor.set(node[0], node[2]);
    // cursor2.set(node[1],node[0]);

    // sine.freq(0);

    // sine2.freq(0);

    initWindow();

    // audio
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    // if gamma
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    scene()->addSource(aSoundSource);
    aSoundSource.dopplerType(DOPPLER_NONE);
    // scene()->usePerSampleProcessing(true);
    scene()->usePerSampleProcessing(false);
  }

  void onAnimate(double dt) {
    while (InterfaceServerClient::oscRecv().recv())
      ;  // XXX
    cursor.update(node);
    // cursor2.update(node);
    state.cursorPosition = cursor.position;
    state.navPosition = nav().pos();
    state.navOrientation = nav().quat();
    maker.set(state);
  }

  void onDraw(Graphics& g) {
    material();
    light();

    for (unsigned i = 0; i < 14; i++) {
      node[i].draw(g, sphere);
    }

    cursor.draw(g, sphere2);
    // cursor2.draw(g, sphere3);

    for (unsigned i = 0; i < struts.size(); i++) {
      struts[i]->draw(g, line);
    }
  }

  SoundSource aSoundSource;
  virtual void onSound(al::AudioIOData& io) {
    aSoundSource.pose(nav());
    while (io()) {
      if (cursor.trigger == true) {
        env.attack(0.01);
        env.decay(0.3);
        env.amp(1.0);
        env.reset();
        cursor.trigger = false;
      }

      float s = 0;

      for (int i = 0; i < 8; i++) {
        sine[i].freq(cursor.currentFrequency * fundamental *
                     timbreFrequency[i]);
        s += sine[i]() * timbreAmplitude[i];
      }

      s *= env() / 8.0f;

      // XXX -- this is broken the line below should work, but it sounds
      // terrible
      // aSoundSource.writeSample(s);
      //
      // these two lines should go onces the lien above works
      io.out(0) = s;
      io.out(1) = s;
    }
    listener()->pose(nav());
    scene()->render(io);
  }
};

int main() {
  AlloApp app;
  app.AlloSphereAudioSpatializer::audioIO().start();
  app.InterfaceServerClient::connect();
  app.maker.start();
  app.start();
}
