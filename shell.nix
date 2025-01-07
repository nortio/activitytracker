{ pkgs ? import <nixpkgs> {}, unstable ? import <nixos-unstable> {} }:
pkgs.mkShell {
	nativeBuildInputs = with pkgs.buildPackages; [
		nodejs_22
		gnumake
		gcc14
		clang-tools
		kdePackages.kcachegrind
		valgrind
		bear
	];
}
