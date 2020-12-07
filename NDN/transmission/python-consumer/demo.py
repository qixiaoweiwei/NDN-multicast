#!/usr/bin/env python2
import client

cl = client.FileClient()
for i in range(10):
    cl.getFile("/File/PDF", "./consumer-files")
