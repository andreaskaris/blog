
mkdir httpd-override
pushd httpd-override
cat <<'EOF' > Dockerfile
FROM docker.io/library/httpd:latest
RUN sed -i 's/AllowOverride None/AllowOverride All/g' /usr/local/apache2/conf/httpd.conf
EOF
sudo buildah bud -t httpd-override
popd

mkdir test-charts
pushd test-charts
helm create test
helm package test
popd
helm repo index test-charts --url http://localhost:8080
sudo mv test-charts/* /var/www/html/
cd /var/www/html
sudo htpasswd -c .htpasswd test

sudo podman run -dit --name my-apache-app -p 8080:80 -v "$PWD":/usr/local/apache2/htdocs/ localhost/httpd-override
