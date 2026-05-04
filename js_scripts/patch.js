/**
 * This script is applied ontop of the environment where C++ classes
 * have been added into the JS context.
 */

// add script methods to prototype
this.Runtime.prototype.__startReady = function() {
    meta = {
      print: (text) => {
        if ((typeof text === "object" || typeof text === "function") && (this.vm != null)) {
          text = this.vm.runner.toString(text);
        }
        return this.listener.log(text);
      }
    };
    global = {
      screen: this.screen.getInterface(),
      audio: this.audio.getInterface(),
      keyboard: this.keyboard.keyboard,
      gamepad: this.gamepad.status,
      sprites: this.sprites,
      sounds: this.sounds,
      music: this.music,
      assets: this.assets,
      asset_manager: this.asset_manager.getInterface(),
      maps: this.maps,
      touch: this.touch,
      mouse: this.mouse,
      fonts: window.fonts,
      Sound: Sound.createSoundClass(this.audio),
      Image: msImage,
      Sprite: Sprite,
      Map: MicroMap
    };
    /**
     * TODO: Inject namespace from project (/<owner>/<slug>/)
     * and see if the namespace is important for VM
     */
    this.vm = new MicroVM(meta, global, '/', false);
}

// finish off environment by booting up the player
window.player = new Player(); player.start();
