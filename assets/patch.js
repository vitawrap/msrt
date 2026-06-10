/**
 * This script is applied ontop of the environment where C++ classes
 * have been added into the JS context.
 */

this.Screen.prototype.getInterface = function() {
  var screen;
  if (this.interface != null) {
    return this.interface;
  }
  screen = this;
  return this.interface = {
    width: this.width,
    height: this.height,
    clear: function(color) {
      return screen.clear(color);
    },
    setColor: function(color) {
      return screen.setColor(color);
    },
    setAlpha: function(alpha) {
      return screen.setAlpha(alpha);
    },
    setPixelated: function(pixelated) {
      return screen.setPixelated(pixelated);
    },
    setBlending: function(blending) {
      return screen.setBlending(blending);
    },
    setLinearGradient: function(x1, y1, x2, y2, c1, c2) {
      return screen.setLinearGradient(x1, y1, x2, y2, c1, c2);
    },
    setRadialGradient: function(x, y, radius, c1, c2) {
      return screen.setRadialGradient(x, y, radius, c1, c2);
    },
    setFont: function(font) {
      return screen.setFont(font);
    },
    setTranslation: function(tx, ty) {
      return screen.setTranslation(tx, ty);
    },
    setScale: function(x, y) {
      return screen.setScale(x, y);
    },
    setRotation: function(rotation) {
      return screen.setRotation(rotation);
    },
    setDrawAnchor: function(ax, ay) {
      return screen.setDrawAnchor(ax, ay);
    },
    setDrawRotation: function(rotation) {
      return screen.setDrawRotation(rotation);
    },
    setDrawScale: function(x, y) {
      return screen.setDrawScale(x, y);
    },
    fillRect: function(x, y, w, h, c) {
      return screen.fillRect(x, y, w, h, c);
    },
    fillRoundRect: function(x, y, w, h, r, c) {
      return screen.fillRoundRect(x, y, w, h, r, c);
    },
    fillRound: function(x, y, w, h, c) {
      return screen.fillRound(x, y, w, h, c);
    },
    drawRect: function(x, y, w, h, c) {
      return screen.drawRect(x, y, w, h, c);
    },
    drawRoundRect: function(x, y, w, h, r, c) {
      return screen.drawRoundRect(x, y, w, h, r, c);
    },
    drawRound: function(x, y, w, h, c) {
      return screen.drawRound(x, y, w, h, c);
    },
    drawSprite: function(sprite, x, y, w, h) {
      return screen.drawSprite(sprite, x, y, w, h);
    },
    drawImage: function(sprite, x, y, w, h) {
      return screen.drawSprite(sprite, x, y, w, h);
    },
    drawSpritePart: function(sprite, sx, sy, sw, sh, x, y, w, h) {
      return screen.drawSpritePart(sprite, sx, sy, sw, sh, x, y, w, h);
    },
    drawImagePart: function(sprite, sx, sy, sw, sh, x, y, w, h) {
      return screen.drawSpritePart(sprite, sx, sy, sw, sh, x, y, w, h);
    },
    drawMap: function(map, x, y, w, h) {
      return screen.drawMap(map, x, y, w, h);
    },
    drawText: function(text, x, y, size, color) {
      return screen.drawText(text, x, y, size, color);
    },
    drawTextOutline: function(text, x, y, size, color) {
      return screen.drawTextOutline(text, x, y, size, color);
    },
    textWidth: function(text, size) {
      return screen.textWidth(text, size);
    },
    setLineWidth: function(width) {
      return screen.setLineWidth(width);
    },
    setLineDash: function(dash) {
      return screen.setLineDash(dash);
    },
    drawLine: function(x1, y1, x2, y2, color) {
      return screen.drawLine(x1, y1, x2, y2, color);
    },
    drawPolygon: function() {
      return screen.drawPolygon(arguments);
    },
    drawPolyline: function() {
      return screen.drawPolyline(arguments);
    },
    fillPolygon: function() {
      return screen.fillPolygon(arguments);
    },
    drawQuadCurve: function() {
      return screen.drawQuadCurve(arguments);
    },
    drawBezierCurve: function() {
      return screen.drawBezierCurve(arguments);
    },
    drawArc: function(x, y, radius, angle1, angle2, ccw, color) {
      return screen.drawArc(x, y, radius, angle1, angle2, ccw, color);
    },
    fillArc: function(x, y, radius, angle1, angle2, ccw, color) {
      return screen.fillArc(x, y, radius, angle1, angle2, ccw, color);
    },
    setCursorVisible: function(visible) {
      return screen.setCursorVisible(visible);
    },
    loadFont: function(font) {
      return screen.loadFont(font);
    },
    isFontReady: function(font) {
      return screen.isFontReady(font);
    }
  };
}

this.Screen.prototype.updateInterface = function() {
  this.interface.width = this.width;
  return this.interface.height = this.height;
}

// extra function to expose sprites but short sprite management back to the C++ side
this.Runtime.prototype.__addSprite = function(path, fps, fcount, width, height) {
  if (this.sprites === undefined) { this.sprites = {}; }
  let fpsField = fps;
  let fcountField = fcount;
  let setFPS = (fps) => { this.__spriteSetFPS(path, fps); };
  let setFrame = (frame) => { this.__spriteSetFrame(path, frame); }
  let getFrame = () => { return this.__spriteGetFrame(path); }
  const name = path.match(/(?<=sprites\/)([^\t\n\r .]+)/g);
  this.sprites[name] = {
    setFPS: setFPS,
    setFrame: setFrame,
    getFrame: getFrame,
    width: width,
    height: height,
    name: name,
    ready: 1
  };
}

// touch interface short circuits to runtime native getters
this.Touch = class {
  constructor(runtime) {
    this.runtime = runtime;
  }

  get touching() { return this.runtime.__touchTouching; }
  get release() { return this.runtime.__touchReleased; }
  get press() { return this.runtime.__touchPressed; }
  get x() { return this.runtime.__touchX; }
  get y() { return this.runtime.__touchY; }
}

// keyboard interface short circuits to runtime native functions
this.Keyboard = function(runtime) {
  let dummy = {};

  let pressHandler = {
    get(target, prop, receiver) {
      return runtime.__keyboardKeyPress(prop.toLowerCase());
    }
  };
  let pressProxy = new Proxy(dummy, pressHandler);

  let releaseHandler = {
    get(target, prop, receiver) {
      return runtime.__keyboardKeyRelease(prop.toLowerCase());
    }
  };
  let releaseProxy = new Proxy(dummy, releaseHandler);

  let ki = class KeyboardInternal {
    constructor() {
      this.press = pressProxy;
      this.release = releaseProxy;
    }
  };
  
  let handler = {
    get(target, prop, receiver) {
      if (prop === 'press' || prop === 'release')
        return Reflect.get(...arguments);
      return runtime.__keyboardKeyDown(prop.toLowerCase());
    },
    ownKeys(target) {
      return [...Object.keys(target), ...runtime.__keyboardKeys];
    }
  };
  return new Proxy(new ki, handler);
}

// add script methods to prototype
this.Runtime.prototype.__startReady = function() {
  var err, file, global, init, j, len1, lib, meta, namespace, ref, ref1, src;
  meta = {
    print: (text) => {
      if ((typeof text === "object" || typeof text === "function") && (this.vm != null)) {
        text = this.vm.runner.toString(text);
      }
      return console.log("[MS] " + text);
    }
  };
  global = {
    screen: this.screen.getInterface(),
    //audio: this.audio.getInterface(),
    keyboard: new Keyboard(this),
    //gamepad: this.gamepad.status,
    sprites: this.sprites,
    sounds: this.sounds,
    music: this.music,
    assets: this.assets,
    //asset_manager: this.asset_manager.getInterface(),
    maps: this.maps,
    touch: new Touch(this),
    mouse: this.mouse,
    fonts: window.fonts,
    //Sound: Sound.createSoundClass(this.audio),
    //Image: msImage,
    //Sprite: Sprite,
    //Map: MicroMap
  };
  /**
   * TODO: Inject namespace from project (/<owner>/<slug>/)
   * and see if the namespace is important for VM
   */
  this.vm = new MicroVM(meta, global, '/', false);

  this.vm.context.global.system.exit = () => {
      return this.exit(); // this calls into our "exit"
  };
  this.vm.context.global.system.disable_autofullscreen = 0;
  this.vm.context.global.system.file = System.file;
  this.vm.context.global.system.javascript = System.javascript;
  System.runtime = this;
  ref1 = this.sources;
  for (file in ref1) {
    src = ref1[file];
    this.updateSource(file, src, false);
  }
  if (this.vm.runner.getFunctionSource != null) {
    init = this.vm.runner.getFunctionSource("init");
    if (init != null) {
      this.previous_init = init;
      this.vm.call("init");
      if (this.vm.error_info != null) {
        err = this.vm.error_info;
        err.type = "draw";
        this.listener.reportError(err);
      }
    }
  } else {
    this.vm.call("init");
    if (this.vm.error_info != null) {
      err = this.vm.error_info;
      err.type = "draw";
      this.listener.reportError(err);
    }
  }
  this.dt = 1000 / 60;
  this.last_time = Date.now();
  this.current_frame = 0;
  this.floating_frame = 0;
  requestAnimationFrame(() => { // this calls into our "requestAnimationFrame"
    return this.timer(); // this calls into our "timer"
  });
  this.screen.startControl();
}

this.Runtime.prototype.__timer = function() {
  time = Date.now();
  if (Math.abs(time - this.last_time) > 160) {
    this.last_time = time - 16;
  }
  dt = time - this.last_time;
  this.dt = this.dt * .9 + dt * .1;
  this.last_time = time;
  this.vm.context.global.system.fps = Math.round(fps = 1000 / this.dt);
  update_rate = this.vm.context.global.system.update_rate;
  if ((update_rate == null) || !(update_rate > 0) || !isFinite(update_rate)) {
    update_rate = 60;
  }
  this.floating_frame += this.dt * update_rate / 1000;
  ds = Math.min(10, Math.round(this.floating_frame - this.current_frame));
  if ((ds === 0 || ds === 2) && update_rate === 60 && Math.abs(fps - 60) < 2) {
    //console.info "INCORRECT DS: "+ds+ " floating = "+@floating_frame+" current = "+@current_frame
    ds = 1;
    this.floating_frame = this.current_frame + 1;
  }
  sync_update = this.vm.context.global.system.sync_update;
  if (sync_update) {
    this.updateCall();
  } else {
    for (i = j = 1, ref = ds; j <= ref; i = j += 1) {
      this.updateCall();
      if (i < ds) {
        if (this.vm.runner.tick != null) {
          this.vm.runner.tick();
        }
      }
    }
  }
  this.current_frame += ds;
  this.drawCall();
  if (this.vm.runner.tick != null) {
    this.vm.runner.tick();
  }
}

this.Runtime.prototype.updateCall = function() {
  var err;
  if (this.vm.runner.triggers_controls_update) {
    if (this.vm.runner.updateControls == null) {
      this.vm.runner.updateControls = () => {
        return this.updateControls();
      };
    }
  } else {
    this.updateControls();
  }
  try {
    //time = Date.now()
    this.vm.call("update");
    //this.reportWarnings();
    //console.info "update time: "+(Date.now()-time)
    if (this.vm.error_info != null) {
      err = this.vm.error_info;
      err.type = "update";
      //return this.listener.reportError(err);
      console.log(err);
    }
  } catch (error) {
    err = error;
    // if (this.report_errors) {
    //   return this.listener.reportError(err);
    // }
    console.log(err, err.stack);
  }
}

this.Runtime.prototype.drawCall = function() {
  var err;
  try {
    this.screen.initDraw();
    this.screen.updateInterface();
    this.vm.call("draw");
    //this.reportWarnings();
    if (this.vm.error_info != null) {
      err = this.vm.error_info;
      err.type = "draw";
      //return this.listener.reportError(err);
      return console.log(err);
    }
  } catch (error) {
    err = error;
    // if (this.report_errors) {
    //   return this.listener.reportError(err);
    // }
    console.log(err, err.stack);
  }
}

this.Runtime.prototype.updateSource = function(file, src, reinit = false) {
  var err, init;
  if (this.vm == null) {
    return false;
  }
  if (src === this.update_memory[file]) {
    return false;
  }
  this.update_memory[file] = src;
  //this.audio.cancelBeeps();
  this.screen.clear();
  try {
    this.vm.run(src, 3000, file);
    // this.listener.postMessage({
    //   name: "compile_success",
    //   file: file
    // });
    //this.reportWarnings();
    if (this.vm.error_info != null) {
      console.error(err);
      err = this.vm.error_info;
      err.type = "init";
      err.file = file;
      //this.listener.reportError(err);
      return false;
    }
    if (this.vm.runner.getFunctionSource != null) {
      init = this.vm.runner.getFunctionSource("init");
      if ((init != null) && init !== this.previous_init && reinit) {
        this.previous_init = init;
        this.vm.call("init");
        if (this.vm.error_info != null) {
          console.error(err);
          err = this.vm.error_info;
          err.type = "init";
          //this.listener.reportError(err);
        }
      }
    }
    return true;
  } catch (error) {
    err = error;
    if (this.report_errors) {
      console.error(err);
      err.file = file;
      //this.listener.reportError(err);
      return false;
    }
  }
}

this.Player.prototype.__resize = function() {
  if (this.runtime.vm != null) {
    if (this.runtime.vm.context.global.draw == null) {
      this.runtime.update_memory = {};
      ref = this.runtime.sources;
      results = [];
      for (file in ref) {
        src = ref[file];
        results.push(this.runtime.updateSource(file, src, false));
      }
      return results;
    } else if (this.runtime.stopped) {
      return this.runtime.drawCall(); // this calls into our "drawCall"
    }
  }
}

this.Player.prototype.__sourceFileAdded = function(file, text) {
  var name = file.split(".")[0];
  this.sources[name] = text;
  this.source_count++;

  // do this here
  this.runtime.sources = this.sources;
}

// Do not create the storage service for now
this.MicroVM.prototype.createStorageService = () => {
  return service = {
    api: {
      set: (name, value) => {},
      get: (name) => {}
    },
    check: () => {}
  };
}

// finish off environment by booting up the player
window.player = new Player(); player.start();
