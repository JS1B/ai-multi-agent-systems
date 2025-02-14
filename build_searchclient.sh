#!/bin/bash

# Navigate to the searchclient directory
cd searchclient_java

# Build the project
javac searchclient/*.java
jar cfm searchclient.jar MANIFEST.MF searchclient/*.class
