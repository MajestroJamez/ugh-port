plugins {
    alias(libs.plugins.kotlin.jvm)
}

kotlin {
    jvmToolchain(25)
}

dependencies {
    implementation(project(":extractor"))
    testImplementation(platform(libs.junit.bom))
    testImplementation(libs.junit.jupiter)
    testRuntimeOnly(libs.junit.launcher)
}

tasks.test {
    useJUnitPlatform()
    systemProperty("ugh.exe", rootProject.layout.projectDirectory.file("OLD/UGH.EXE").asFile.absolutePath)
    systemProperty("ugh.out", layout.buildDirectory.dir("oracle-out").get().asFile.absolutePath)
    maxHeapSize = "2g"
    testLogging { showStandardStreams = true }
}
