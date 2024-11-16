# rf2

> A tiny and simple file finder

## About

__This project is a restart of my original implementation: https://github.com/roryrjb/rf, this time only targetting Windows.__

rf is a simple command line tool for Windows. It's main purpose is to print a list of one or more files based on glob patterns. It's obviously very similar to other options out there, the main difference is the optional use of .rfignore files. These work like .gitignore files but are separate so it's agnostic to your development workflow. rf will look for an .rfignore file in your home directory and in the current working directory, ignoring any patterns you specify.

This is currently a work in progress.