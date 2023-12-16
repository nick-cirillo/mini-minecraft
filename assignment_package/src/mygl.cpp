#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDateTime>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progSky(this),
      m_geomQuad(this), m_terrain(this),
      m_player(glm::vec3(64.f, 150.f, 64.f), m_terrain),m_lastMSecs(QDateTime::currentMSecsSinceEpoch()),
      m_time(0.f),
      m_buffer(this,this->width(),this->height(),this->devicePixelRatio()), m_postProcessShaders()
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    m_geomQuad.destroyVBOdata();
    m_buffer.destroy();
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Activate transparency using out_Color.w
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    m_buffer.create();//creating buffer data

    m_geomQuad.createVBOdata();
    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    //m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");

    // Sky shader
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
    m_geomQuad.createVBOdata();

    printGLErrorLog();

    std::shared_ptr<PostProcessShader> s = std::make_shared<PostProcessShader>(this);
    s->create(":/glsl/passthrough.vert.glsl",":/glsl/noOp.frag.glsl");
    s->setupMemberVars();
    m_postProcessShaders.push_back(s);
    m_progPostprocessCurrent = s.get();

    s = std::make_shared<PostProcessShader>(this);
    s->create(":/glsl/passthrough.vert.glsl",":/glsl/water.frag.glsl");
    s->setupMemberVars();
    m_postProcessShaders.push_back(s);

    s = std::make_shared<PostProcessShader>(this);
    s->create(":/glsl/passthrough.vert.glsl",":/glsl/lava.frag.glsl");
    s->setupMemberVars();
    m_postProcessShaders.push_back(s);


    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    // Set texture map once in MyGL so it isn't reinitialized for every chunk
    m_textureMap = std::unique_ptr<Texture>(new Texture(this));
    m_textureMap->create(":/textures/minecraft_textures_all.png");
    m_textureMap->load(0);
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_buffer.resize(w*this->devicePixelRatio(),h*this->devicePixelRatio(),1);
    m_buffer.destroy();
    m_buffer.create();
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    // Sky shader
    m_progSky.setViewProjMatrix(glm::inverse(viewproj));



    m_progPostprocessCurrent->setDimensions(glm::ivec2(w * this->devicePixelRatio(),
                                                       h * this->devicePixelRatio()));
    //TODO: do this for all post process shaders




    // Sky shader
    m_progSky.useMe();
    this->glUniform2i(m_progSky.unifDimensions, width(), height());
    this->glUniform3f(m_progSky.unifEye, m_player.mcr_position.x, m_player.mcr_position.y, m_player.mcr_position.z);


    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    m_terrain.checkForNewChunks();
    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    playerTick(); // Calculates dT and calls Player::tick()
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data

    m_progPostprocessCurrent = m_postProcessShaders[m_player.medium].get();
}

void MyGL::playerTick() {
    qint64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
    float dT = (currentMSecs - m_lastMSecs) / 1000.f;
    m_player.tick(dT, m_inputs);
    m_lastMSecs = currentMSecs;
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

void MyGL::bindTextureMap() {
    m_textureMap->bind(0);
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    //frame buffer stuff here
    m_buffer.bindFrameBuffer();
    glViewport(0,0,this->width()*this->devicePixelRatio(),this->height()*this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());

    // Sky shader
   //paste back here
    glDisable(GL_DEPTH_TEST);
    m_progSky.setViewProjMatrix(glm::inverse(m_player.mcr_camera.getViewProj()));
    m_progSky.useMe();
    this->glUniform3f(m_progSky.unifEye, m_player.mcr_position.x, m_player.mcr_position.y, m_player.mcr_position.z);
    this->glUniform1f(m_progSky.unifTimeSky, timeSky++);
    m_progSky.draw(m_geomQuad);
    float theta = timeSky * 0.001 * 3.14159265359;
    sunDir = glm::vec3(0, cos(theta), sin(theta));
    sunDir = glm::normalize(sunDir);
    glEnable(GL_DEPTH_TEST);

    printGLErrorLog();

    m_progLambert.setTime(m_time);
    m_progPostprocessCurrent->setTime(m_time);
    m_time++;

    m_progLambert.useMe();
    this->glUniform3f(m_progLambert.unifSunDir, sunDir.x, sunDir.y, sunDir.z);

    printGLErrorLog();
    renderTerrain();

    //post process render pass now
    glBindFramebuffer(GL_FRAMEBUFFER,this->defaultFramebufferObject());
    glViewport(0,0,this->width()*this->devicePixelRatio(),this->height()*this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_buffer.bindToTextureSlot(1);

//    this->glUniform1i(m_progPostprocessCurrent->unifSampler2D,m_buffer.getTextureSlot());

    m_progPostprocessCurrent->draw(m_geomQuad,m_buffer.getTextureSlot());

    // pasted sky code - move up



//    glDisable(GL_DEPTH_TEST);
//    m_progFlat.setModelMatrix(glm::mat4());
//    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
//    m_progFlat.draw(m_worldAxes);
//    glEnable(GL_DEPTH_TEST);
}

void MyGL::renderTerrain() {
    bindTextureMap();

    glm::vec2 terrainPos = m_terrain.getTerrainPos();

    int terrX = static_cast<int>(terrainPos.x);
    int terrZ = static_cast<int>(terrainPos.y);

    for (int z = terrZ - 64; z < terrZ + 128; z += 64) {
        for (int x = terrX - 64; x < terrX + 128; x += 64) {
            if (m_terrain.hasTerrainZoneAt(x, z)) {
                m_terrain.draw(x, x + 64, z, z + 64, &m_progLambert);
            } else {
                // Edit the CreateProceduralTerrain function to
                // create more terrain for this generation zone
                m_terrain.CreateProceduralTerrain(x,x+64,z,z+64);
                m_terrain.draw(x, x + 64, z, z + 64, &m_progLambert);
            }
        }
    }
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
        case Qt::Key_Escape:
            QApplication::quit();
            break;

        case Qt::Key_W:
            m_inputs.wPressed = true;
            break;
        case Qt::Key_S:
            m_inputs.sPressed = true;
            break;
        case Qt::Key_D:
            m_inputs.dPressed = true;
            break;
        case Qt::Key_A:
            m_inputs.aPressed = true;
            break;
        case Qt::Key_Q:
            m_inputs.qPressed = true;
            break;
        case Qt::Key_E:
            m_inputs.ePressed = true;
            break;
        case Qt::Key_F:
            if (!m_player.getFlightModeSet()) {
                m_player.setFlightMode(!m_player.getFlightMode());
                m_player.setFlightModeSet(true);
            }
            break;
        case Qt::Key_Space:
            if (!m_player.getFlightMode() && !m_player.isJumping()) {
                m_player.setJumping(true);
            }
            break;
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    switch(e->key()) {
        case Qt::Key_W:
            m_inputs.wPressed = false;
            break;
        case Qt::Key_S:
            m_inputs.sPressed = false;
            break;
        case Qt::Key_D:
            m_inputs.dPressed = false;
            break;
        case Qt::Key_A:
            m_inputs.aPressed = false;
            break;
        case Qt::Key_Q:
            m_inputs.qPressed = false;
            break;
        case Qt::Key_E:
            m_inputs.ePressed = false;
            break;
        case Qt::Key_F:
            m_player.setFlightModeSet(false);
            break;
        case Qt::Key_Right:
            m_player.rotateOnUpGlobal(-2);
            break;
        case Qt::Key_Left:
            m_player.rotateOnUpGlobal(2);
            break;
        case Qt::Key_Up:
            m_player.rotateOnRightLocal(2);
            break;
        case Qt::Key_Down:
            m_player.rotateOnRightLocal(-2);
            break;
        case Qt::Key_Space:
            if (!m_player.getFlightMode() && m_player.isJumping() && m_player.isSwimming()) {
                m_player.setJumping(false);
            }
            break;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    // TODO
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    switch (e->buttons()) {
        case Qt::LeftButton:
            m_player.removeBlock(m_terrain);
            break;
        case Qt::RightButton:
            m_player.addBlock(m_terrain);
            break;
    }
}
