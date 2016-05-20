@echo off

call "%VC%\vcvarsall.bat" amd64
gvim code/*
