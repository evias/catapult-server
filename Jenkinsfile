pipeline {
    agent any
    environment {
        BOOST_URL = 'https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.gz',
        GTEST_URL = 'https://github.com/google/googletest/archive/release-1.8.1.tar.gz',
        GBENCH_URL = 'https://github.com/google/benchmark/archive/v1.5.0.tar.gz',
        MONGOC_URL = 'https://github.com/mongodb/mongo-c-driver/releases/download/1.15.1/mongo-c-driver-1.15.1.tar.gz',
        MONGOCXX_URL = 'https://github.com/nemtech/mongo-cxx-driver/archive/r3.4.0-nem.tar.gz',
        ZMQ_URL = 'https://github.com/zeromq/libzmq/releases/download/v4.3.2/zeromq-4.3.2.tar.gz',
        ZMQCPP_URL = 'https://github.com/zeromq/cppzmq/archive/v4.4.1.tar.gz',
        ROCKS_URL = 'https://github.com/nemtech/rocksdb/archive/v6.2.4-nem.tar.gz'
    }
    node {
        def workspace = pwd()
        def path_archives = "${workspace}/_archives"
        def path_deps = "${workspace}/_deps"
        def path_build = "${workspace}/_build"
    }
    stages {
        stage('Download dependencies') {
            agent any
            steps {
                sh "cd ${workspace}"
                sh "mkdir _archives && cd _archives"
                sh "curl -o boost_1_71_0.tar.gz -SL ${env.BOOST_URL}"
                sh "curl -o gtest_1_8_1.tar.gz -SL ${env.GTEST_URL}"
                sh "curl -o gbench_1_5_0.tar.gz -SL ${env.GBENCH_URL}"
                sh "curl -o mongoc_1_15_1.tar.gz -SL ${env.MONGOC_URL}"
                sh "curl -o mongocxx_3_4_0_nem.tar.gz -SL ${env.MONGOCXX_URL}"
                sh "curl -o zmq_4_3_2.tar.gz -SL ${env.ZMQ_URL}"
                sh "curl -o zmqcpp_4_4_1.gz -SL ${env.ZMQCPP_URL}"
                sh "curl -o rocks_6_2_4_nem.gz -SL ${env.ROCKS_URL}"

                stash includes: '_archives/*', name: 'archives' 
            }
        }
        stage('Extract dependencies') {
            agent any
            steps {
                unstash 'archives'

                sh "cd ${workspace}"
                sh "mkdir _deps && cd _deps"
                sh "tar -xzf boost_1_71_0.tar.gz"
                sh "tar -xzf gtest_1_8_1.tar.gz"
                sh "tar -xzf gbench_1_5_0.tar.gz"
                sh "tar -xzf mongoc_1_15_1.tar.gz"
                sh "tar -xzf mongocxx_3_4_0_nem.tar.gz"
                sh "tar -xzf zmq_4_3_2.tar.gz"
                sh "tar -xzf zmqcpp_4_4_1.gz"
                sh "tar -xzf rocks_6_2_4_nem.gz"

                stash includes: '_deps/*', name: 'dependencies'
            }
        }
        stage('Build dependencies') {
            agent any
            steps {
                unstash 'dependencies' 
                sh "cd ${path_deps}"

                // 1. build boost
                sh "mkdir boost-build-1.71.0"
                sh "cd boost_1_71_0"
                sh "./bootstrap.sh --prefix=${path_deps}/boost-build-1.71.0"
                sh "./b2 --prefix=${path_deps}/boost-build-1.71.0 --without-python -j 4 stage release"
                sh "./b2 --prefix=${path_deps}/boost-build-1.71.0 --without-python install"
                sh "cd ${path_deps}"

                // 2. build google test
                sh "cd gtest_1_8_1"
                sh "mkdir _build && cd _build"
                sh "cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON .."
                sh "make && make install"
                sh "cd ${path_deps}"

                // 3. build google benchmark
                sh "cd gbench_1_5_0"
                sh "mkdir _build && cd _build"
                sh "cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF .."
                sh "make && make install"
                sh "cd ${path_deps}"

                // 4. build mongo-c-driver
                sh "cd mongoc_1_15_1"
                sh "mkdir _build && cd _build"
                sh "cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .."
                sh "make && make install"
                sh "cd ${path_deps}"

                // 5. build mongo-cxx-driver@r3.4.0-nem
                sh "cd mongocxx_3_4_0_nem"
                sh "mkdir _build && cd _build"
                sh "cmake -DCMAKE_CXX_STANDARD=17 -DLIBBSON_DIR=/usr/local -DLIBMONGOC_DIR=/usr/local -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .."
                sh "make && make install"
                sh "cd ${path_deps}"

                // 6. build libzmq
                sh "cd zmq_4_3_2"
                sh "mkdir _build && cd _build"
                sh "cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .."
                sh "make && make install"
                sh "cd ${path_deps}"

                // 7. build cppzmq
                sh "cd zmqcpp_4_4_1"
                sh "mkdir _build && cd _build"
                sh "cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCPPZMQ_BUILD_TESTS=OFF .."
                sh "make && make install"
                sh "cd ${path_deps}"

                // 8. build rocksdb@v6.2.4-nem
                sh "cd rocks_6_2_4_nem"
                sh "mkdir _build && cd _build"
                sh "cmake -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local .."
                sh "make && make install"
                sh "cd ${path_deps}"
            }
        }
        stage('Build protocol') {
            agent any
            steps {
                sh "cd ${workspace}"

                checkout scm
                sh 'mkdir _build && cd _build'
                sh "cmake -DBOOST_ROOT=${path_deps}/boost-build-1.71.0 -DCMAKE_BUILD_TYPE=Release -G Ninja .."
                sh 'ninja publish'
                sh 'ninja -j4'
                stash includes: '_build/*', name: 'protocol' 
            }
        }
        stage('Test on Linux') {
            agent {
                label 'linux'
            }
            steps {
                unstash 'protocol'
                sh 'make check'
            }
            post {
                always {
                    junit '**/target/*.xml'
                }
            }
        }
    }
}