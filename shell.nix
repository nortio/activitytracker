{ pkgs ? import <nixpkgs> {}, unstable ? import <nixos-unstable> {} }:
pkgs.mkShell {
	nativeBuildInputs = with pkgs.buildPackages; [
		nodejs_22
		gnumake
		gcc14
		clang-tools
		clang_19
		kdePackages.kcachegrind
		valgrind
		bear
		#llvmPackages_19.libcxx.dev
		boost.dev
	];
}
