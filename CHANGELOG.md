# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

### Added
- Documentation for public API
- `row_range` to simplify use of `row_iterator`

### Fixed
- `row_iterator` and `row` now properly implement range concepts
- Removed unused and invalid #ifdefs for configuration dependent methods of the statement
- Made handle::c_ptr(T *) an inline friend as it should have been 

## [1.1] - 2023-07-30

### Added
- Ability to install library locally

### Changed
- Modernized and cleaned up CMake configuration

### Fixed
- Warnings on various versions of gcc and clang

## [1.0] - 2022-05-18

### Added
- Initial version

[1.0]: https://github.com/gershnik/thinsqlitepp/releases/v1.0
[1.1]: https://github.com/gershnik/thinsqlitepp/releases/v1.1
