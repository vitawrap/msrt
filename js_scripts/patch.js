/**
 * This script is applied ontop of the environment where C++ classes
 * have been added into the JS context.
 */

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
    //screen: this.screen.getInterface(),
    //audio: this.audio.getInterface(),
    //keyboard: this.keyboard.keyboard,
    //gamepad: this.gamepad.status,
    sprites: this.sprites,
    sounds: this.sounds,
    music: this.music,
    assets: this.assets,
    //asset_manager: this.asset_manager.getInterface(),
    maps: this.maps,
    touch: this.touch,
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
