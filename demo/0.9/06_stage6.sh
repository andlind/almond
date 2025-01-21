docker build -t webapp2 webapp2
nohup docker run --rm --platform linux/amd64 --net almond-redpanda-quickstart_almond-demo --name web_demo2 -d -v ~/almond_demo/data:/opt/almond/data -p 8074:80 webapp2
echo "Stage 1 done"
