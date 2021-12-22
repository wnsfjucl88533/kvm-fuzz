usingnamespace @import("../common.zig");
const fs = @import("../../fs/fs.zig");
const mem = @import("../../mem/mem.zig");
const UserPtr = mem.safe.UserPtr;
const UserSlice = mem.safe.UserSlice;

fn sys_socket(self: *Process, domain: i32, type_: i32, protocol: i32) !linux.fd_t {
    const fd = self.availableFd() orelse return error.NoFdAvailable;
    const socket_type = .{
        .domain = domain,
        .type_ = type_,
        .protocol = protocol,
    };
    const file = try fs.file_manager.openSocket(self.allocator, socket_type);
    errdefer file.ref.unref();
    try self.files.table.put(fd, file);
    return fd;
}

pub fn handle_sys_socket(self: *Process, arg0: usize, arg1: usize, arg2: usize) !usize {
    const domain = std.meta.cast(i32, arg0);
    const type_ = std.meta.cast(i32, arg1);
    const protocol = std.meta.cast(i32, arg2);
    const ret = try sys_socket(self, domain, type_, protocol);
    return std.meta.cast(usize, ret);
}

fn sys_bind(
    self: *Process,
    sockfd: linux.fd_t,
    addr_ptr: UserPtr(*const linux.sockaddr),
    addr_len: linux.socklen_t,
) !void {
    const file = self.files.table.get(sockfd) orelse return error.BadFD;
    const socket = file.socket() orelse return error.NotSocket;
    return socket.bind(addr_ptr, addr_len);
}

pub fn handle_sys_bind(self: *Process, arg0: usize, arg1: usize, arg2: usize) !usize {
    const sockfd = std.meta.cast(linux.fd_t, arg0);
    const addr_ptr = try UserPtr(*const linux.sockaddr).fromFlat(arg1);
    const addr_len = std.meta.cast(linux.socklen_t, arg2);
    try sys_bind(self, sockfd, addr_ptr, addr_len);
    return 0;
}

fn sys_listen(self: *Process, sockfd: linux.fd_t, backlog: i32) !void {
    const file = self.files.table.get(sockfd) orelse return error.BadFD;
    const socket = file.socket() orelse return error.NotSocket;
    return socket.listen(backlog);
}

pub fn handle_sys_listen(self: *Process, arg0: usize, arg1: usize) !usize {
    const sockfd = std.meta.cast(linux.fd_t, arg0);
    const backlog = std.meta.cast(i32, arg0);
    try sys_listen(self, sockfd, backlog);
    return 0;
}

fn sys_accept(
    self: *Process,
    sockfd: linux.fd_t,
    addr_ptr: ?UserPtr(*linux.sockaddr),
    addr_len_ptr: ?UserPtr(*linux.socklen_t),
) !linux.fd_t {
    const file = self.files.table.get(sockfd) orelse return error.BadFD;
    const socket = file.socket() orelse return error.NotSocket;
    if (!socket.binded or !socket.listening) // bind shouldn't be necessary?
        return error.InvalidArgument;

    // Create the new socket
    const accepted_socket_file = try fs.file_manager.openSocket(self.allocator, socket.socket_type);
    errdefer accepted_socket_file.ref.unref();
    const accepted_socket = accepted_socket_file.socket().?;
    accepted_socket.connected = true;

    // TODO: write into addr_ptr and addr_len_ptr

    // Insert new socket in our file descriptor table
    const accepted_fd = self.availableFd() orelse return error.NoFdAvailable;
    try self.files.table.put(accepted_fd, accepted_socket_file);
    return accepted_fd;
}

pub fn handle_sys_accept(
    self: *Process,
    arg0: usize,
    arg1: usize,
    arg2: usize,
) !usize {
    const sockfd = std.meta.cast(linux.fd_t, arg0);
    const addr_ptr = UserPtr(*linux.sockaddr).fromFlatMaybeNull(arg1);
    const addr_len_ptr = UserPtr(*linux.socklen_t).fromFlatMaybeNull(arg2);
    const ret = try sys_accept(self, sockfd, addr_ptr, addr_len_ptr);
    return std.meta.cast(usize, ret);
}

fn sys_recv(
    self: *Process,
    sockfd: linux.fd_t,
    buf: UserSlice([]u8),
    flags: linux.i32,
) !usize {
    return sys_recvfrom(self, sockfd, buf, flags, null, null);
}

pub fn handle_sys_recv(
    self: *Process,
    arg0: usize,
    arg1: usize,
    arg2: usize,
    arg3: usize,
) !usize {
    const sockfd = std.meta.cast(linux.fd_t, arg0);
    const buf = try UserSlice([]u8).fromFlat(arg1, arg2);
    const flags = std.meta.cast(i32, arg3);
    return sys_recv(self, sockfd, buf, flags);
}

fn sys_recvfrom(
    self: *Process,
    sockfd: linux.fd_t,
    buf: UserSlice([]u8),
    flags: i32,
    src_addr_ptr: ?UserPtr(*linux.sockaddr),
    addr_len_ptr: ?UserPtr(*linux.socklen_t),
) !usize {
    const file = self.files.table.get(sockfd) orelse return error.BadFD;
    _ = file.socket() orelse return error.NotSocket;
    if (!file.isReadable())
        return error.BadFD;
    return file.read(file, buf);
}

pub fn handle_sys_recvfrom(
    self: *Process,
    arg0: usize,
    arg1: usize,
    arg2: usize,
    arg3: usize,
    arg4: usize,
    arg5: usize,
) !usize {
    const sockfd = std.meta.cast(linux.fd_t, arg0);
    const buf = try UserSlice([]u8).fromFlat(arg1, arg2);
    const flags = std.meta.cast(i32, arg3);
    const src_addr_ptr = UserPtr(*linux.sockaddr).fromFlatMaybeNull(arg4);
    const addr_len = UserPtr(*linux.socklen_t).fromFlatMaybeNull(arg5);
    return sys_recvfrom(self, sockfd, buf, flags, src_addr_ptr, addr_len);
}
