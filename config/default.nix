{ pkgs ?  import <nixpkgs> {}
, firmware ? import ../src {}
}:

let
  config = ./.;
  gesture_module = ../modules/gesture_processor;

  go60_left  = (firmware.zmk.override { 
    board = "go60_lh"; 
    keymap = "${config}/go60.keymap"; 
    kconfig = "${config}/go60.conf"; 
  }).overrideAttrs (oldAttrs: {
    ZMK_EXTRA_MODULES = gesture_module;
  });

  go60_right = (firmware.zmk.override { 
    board = "go60_rh"; 
    keymap = "${config}/go60.keymap"; 
    kconfig = "${config}/go60.conf"; 
  }).overrideAttrs (oldAttrs: {
    ZMK_EXTRA_MODULES = gesture_module;
  });

in firmware.combine_uf2 go60_left go60_right "go60"
