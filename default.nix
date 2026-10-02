# Alea - Nix recipe
#
# Classic (flake-free) Nix expression. nixpkgs is pinned to the nixos-25.11
# release below so the environment is reproducible; pass your own `pkgs` to
# override it, e.g. `--arg pkgs 'import <nixpkgs> {}'`.
#
# The release ships cmake 4.1.2 (project minimum: 3.28) and gcc 14. The tests
# use the vendored doctest submodule (v2.5.3, cmake_minimum_required 3.14),
# which builds fine under cmake 4.x without any policy workaround.
#
# Exposed attributes:
#   alea                    GCC build, checkPhase runs the unit tests
#   aleaWithValidation      GCC build, checkPhase also runs the validation
#                           tests against the pinned random123 reference
#   aleaClang               same as alea, built with clang
#   aleaWithValidationClang same as aleaWithValidation, built with clang
#
# `alea` is a normal derivation and can be overridden like any other nixpkgs
# package, for example to enable the benchmarks:
#
#   alea.override { benchmarks = true; }
#
# Examples:
#   nix-build default.nix -A alea
#   nix-build default.nix -A aleaWithValidation
{ pkgs ? import (builtins.fetchTarball {
    # nixpkgs 25.11 ("Xantusia"), released 2025-11-30
    name = "nixpkgs-25.11";
    url = "https://github.com/NixOS/nixpkgs/archive/871b9fd269ff6246794583ce4ee1031e1da71895.tar.gz";
    sha256 = "1zn1lsafn62sz6azx6j735fh4vwwghj8cc9x91g5sx2nrg23ap9k";
  }) { }
}:

let
  # Reference random123 implementation the validation tests compare against.
  # The CMakeLists.txt normally fetches it at configure time with
  # FetchContent; we pre-fetch it here instead so that the revision is pinned
  # and the build works offline, inside the nix sandbox. Passing
  # `-DFETCHCONTENT_SOURCE_DIR_RANDOM123=<this path>` makes FetchContent use
  # it without cloning anything.
  random123 = builtins.fetchTarball {
    name = "random123-v1.14.0";
    url = "https://github.com/DEShawResearch/random123/archive/726a093cd9a73f3ec3c8d7a70ff10ed8efec8d13.tar.gz"; # v1.14.0
    sha256 = "1awsf27k3mrp8rlc6wqzm2a5lndggp18fvzir395b3hljyx45bxg";
  };

  # A plain derivation built with the default (GCC) stdenv. `makeOverridable`
  # exposes `.override`, so the toolchain (stdenv) and the optional test
  # suites can be changed from the outside without any special plumbing.
  mkAlea = pkgs.lib.makeOverridable
    ({ stdenv ? pkgs.stdenv, validation ? false, benchmarks ? false }:
      stdenv.mkDerivation {
        pname = "alea";
        version = "0.1.0";

        src = pkgs.lib.cleanSourceWith {
          src = ./.;
          # keep the tree deterministic: drop VCS metadata and any local
          # build directory; the submodules (third_party/doctest and
          # third_party/nanobench) are kept, they are needed to build the
          # tests and the benchmarks.
          filter = path: type:
            pkgs.lib.cleanSourceFilter path type
            && !(builtins.elem (baseNameOf path) [ "build" "build-agent" "build-agent-san" ]);
        };

        # the cmake setup hook selects ninja as the generator automatically.
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];

        # tests are ON by default at the top level, no need to pass
        # ALEA_ENABLE_TESTS explicitly.
        cmakeFlags = [
          "-DALEA_ENABLE_VALIDATION=${if validation then "ON" else "OFF"}"
          "-DALEA_ENABLE_BENCH=${if benchmarks then "ON" else "OFF"}"
        ] ++ pkgs.lib.optionals validation [
          # give the reference implementation to FetchContent instead of
          # letting it clone from github at configure time
          "-DFETCHCONTENT_SOURCE_DIR_RANDOM123=${random123}"
        ];

        # checkPhase (provided by the cmake setup hook) runs `ctest`, i.e. the
        # unit tests, plus the validation tests when validation is enabled.
        doCheck = true;

        meta = with pkgs.lib; {
          description = "Modern counter based pseudorandom generators in C++20";
          homepage = "https://github.com/adevress/alea";
          license = licenses.boost;
          platforms = platforms.linux;
        };
      });

  alea = mkAlea { };
  aleaClang = alea.override { stdenv = pkgs.clangStdenv; };
in
{
  inherit random123;

  alea = alea;
  aleaClang = aleaClang;

  aleaWithValidation = alea.override { validation = true; };
  aleaWithValidationClang = aleaClang.override { validation = true; };
}
