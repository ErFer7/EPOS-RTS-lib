#!/bin/bash
echo "Generating submission.diff"/
make clean
make veryclean
git diff main > submission.diff
echo "Submission.diff generated"