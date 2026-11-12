import { execFile } from 'node:child_process'
import { access, mkdir, readFile, readdir, unlink, writeFile } from 'node:fs/promises'
import path from 'node:path'
import { promisify } from 'node:util'
import { defineConfig, type Plugin } from 'vite'
import react from '@vitejs/plugin-react'
import mammoth from 'mammoth'

const execFileAsync = promisify(execFile)
const projectRoot = path.resolve(process.cwd(), '..')
const localRagBinary = path.resolve(projectRoot, 'build-desktop-final', 'localrag.exe')
const importedFilesRoot = path.resolve(projectRoot, 'data', 'imported')

type JsonRecord = Record<string, unknown>

function sendJson(response: any, status: number, body: JsonRecord) {
  response.statusCode = status
  response.setHeader('Content-Type', 'application/json; charset=utf-8')
  response.end(JSON.stringify(body))
}

async function readJson(request: any): Promise<JsonRecord> {
  const chunks: Buffer[] = []
  let size = 0
  for await (const chunk of request) {
    const buffer = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk)
    size += buffer.length
    if (size > 32 * 1024 * 1024) throw new Error('请求体超过 32MB 限制')
    chunks.push(buffer)
  }
  const raw = Buffer.concat(chunks).toString('utf8')
  return raw ? JSON.parse(raw) as JsonRecord : {}
}

async function runLocalRag(args: string[], env: Record<string, string> = {}) {
  return execFileAsync(localRagBinary, args, {
    cwd: projectRoot,
    env: { ...process.env, ...env },
    windowsHide: true,
    maxBuffer: 4 * 1024 * 1024,
  })
}

function parseStats(output: string) {
  const read = (name: string) => Number(output.match(new RegExp(`${name}:\\s*(\\d+)`))?.[1] ?? 0)
  return { documents: read('documents'), chunks: read('chunks'), messages: read('messages'), retrievalLogs: read('retrieval_logs') }
}

function parseDocuments(output: string) {
  return output.split(/\r?\n/).map((line) => line.trim()).filter(Boolean).map((line) => {
    const [id, title, filePath] = line.split(' | ')
    if (!id || !title || !filePath) return null
    return { id: `local-${id}`, name: title.replace(/^\d+-/, ''), meta: path.extname(filePath).toLowerCase() + ' · 已入库', accent: '#73d3ff' }
  }).filter((document): document is { id: string; name: string; meta: string; accent: string } => Boolean(document))
}

function safeFileName(name: string) {
  const extension = path.extname(name).toLowerCase()
  const stem = path.basename(name, extension).replace(/[^\w\u4e00-\u9fff.-]+/g, '_').slice(0, 80) || 'document'
  return `${Date.now()}-${stem}${extension}`
}

function localCliPath(filePath: string) {
  // Keep the path passed to the Windows CLI relative to the project root.
  // Absolute paths inherit the Chinese workspace name and can fail when the
  // C++ executable converts its narrow argv value into a filesystem path.
  return path.relative(projectRoot, filePath)
}

function localRagApi(): Plugin {
  return {
    name: 'localrag-local-api',
    configureServer(server) {
      server.middlewares.use(async (request, response, next) => {
        const url = request.url ?? ''
        if (!url.startsWith('/api/')) {
          next()
          return
        }

        try {
          if (url === '/api/localrag/stats' && request.method === 'GET') {
            const result = await runLocalRag(['stats'])
            sendJson(response, 200, { stats: parseStats(result.stdout) })
            return
          }

          if (url === '/api/localrag/documents' && request.method === 'GET') {
            const result = await runLocalRag(['list-docs'])
            sendJson(response, 200, { documents: parseDocuments(result.stdout) })
            return
          }

          if (url === '/api/localrag/import' && request.method === 'POST') {
            const body = await readJson(request)
            const name = String(body.name ?? '')
            const content = String(body.content ?? '')
            const extension = path.extname(name).toLowerCase()
            if (!['.md', '.txt', '.docx'].includes(extension)) {
              sendJson(response, 400, { error: '目前支持 Markdown、TXT 和 Word（.docx）文件' })
              return
            }
            if (!content) {
              sendJson(response, 400, { error: '不能导入空文件' })
              return
            }
            let plainText = content
            if (extension === '.docx') {
              const result = await mammoth.extractRawText({ buffer: Buffer.from(content, 'base64') })
              plainText = result.value
            }
            if (!plainText.trim()) {
              sendJson(response, 400, { error: '文件中没有可检索的文字内容' })
              return
            }
            await mkdir(importedFilesRoot, { recursive: true })
            const filePath = path.join(importedFilesRoot, safeFileName(name.replace(/\.docx$/i, '.txt')))
            await writeFile(filePath, plainText, 'utf8')
            let result
            try {
              result = await runLocalRag(['import', localCliPath(filePath)])
            } catch (error) {
              await unlink(filePath).catch(() => undefined)
              throw error
            }
            const chunks = Number(result.stdout.match(/imported chunks:\s*(\d+)/i)?.[1] ?? 0)
            sendJson(response, 200, {
              document: {
                id: `local-${Date.now()}`,
                name: path.basename(name, extension),
                meta: `${(Buffer.byteLength(plainText, 'utf8') / 1024).toFixed(1)} KB · ${chunks} chunks`,
                accent: '#73d3ff',
              },
              stats: parseStats((await runLocalRag(['stats'])).stdout),
            })
            return
          }

          if (url === '/api/localrag/delete' && request.method === 'POST') {
            const body = await readJson(request)
            const rawId = String(body.id ?? '').replace(/^local-/, '')
            const documentId = Number(rawId)
            if (!Number.isInteger(documentId) || documentId <= 0) {
              sendJson(response, 400, { error: '文档 ID 无效' })
              return
            }
            const listing = await runLocalRag(['list-docs'])
            const match = listing.stdout.split(/\r?\n/).map((line) => line.trim()).find((line) => line.startsWith(`${documentId} | `))
            if (!match) {
              sendJson(response, 404, { error: '文档不存在' })
              return
            }
            const filePath = match.split(' | ').slice(2).join(' | ')
            await runLocalRag(['delete', String(documentId)])
            const resolvedPath = path.resolve(projectRoot, filePath)
            if (resolvedPath.startsWith(`${importedFilesRoot}${path.sep}`)) await unlink(resolvedPath).catch(() => undefined)
            sendJson(response, 200, { deletedId: `local-${documentId}`, stats: parseStats((await runLocalRag(['stats'])).stdout) })
            return
          }

          if (url === '/api/localrag/clear' && request.method === 'POST') {
            await runLocalRag(['clear'])
            const files = await readdir(importedFilesRoot).catch(() => [])
            await Promise.all(files.map((file) => unlink(path.join(importedFilesRoot, file))))
            sendJson(response, 200, { stats: parseStats((await runLocalRag(['stats'])).stdout) })
            return
          }

          if (url === '/api/localrag/document' && request.method === 'POST') {
            const body = await readJson(request)
            const rawId = String(body.id ?? '').replace(/^local-/, '')
            const documentId = Number(rawId)
            if (!Number.isInteger(documentId) || documentId <= 0) {
              sendJson(response, 400, { error: '文档 ID 无效' })
              return
            }
            const listing = await runLocalRag(['list-docs'])
            const match = listing.stdout.split(/\r?\n/).map((line) => line.trim()).find((line) => line.startsWith(`${documentId} | `))
            if (!match) {
              sendJson(response, 404, { error: '文档不存在' })
              return
            }
            const filePath = path.resolve(projectRoot, match.split(' | ').slice(2).join(' | '))
            try {
              await access(filePath)
            } catch {
              sendJson(response, 404, { error: '源文件已被移动或删除' })
              return
            }
            const content = await readFile(filePath, 'utf8')
            sendJson(response, 200, {
              document: {
                id: `local-${documentId}`,
                name: path.basename(match.split(' | ')[1]),
                meta: `${path.extname(filePath).toLowerCase()} · ${Buffer.byteLength(content, 'utf8')} bytes`,
              },
              content,
            })
            return
          }

          if (url === '/api/localrag/open' && request.method === 'POST') {
            const body = await readJson(request)
            const rawId = String(body.id ?? '').replace(/^local-/, '')
            const documentId = Number(rawId)
            if (!Number.isInteger(documentId) || documentId <= 0) {
              sendJson(response, 400, { error: '文档 ID 无效' })
              return
            }
            const listing = await runLocalRag(['list-docs'])
            const match = listing.stdout.split(/\r?\n/).map((line) => line.trim()).find((line) => line.startsWith(`${documentId} | `))
            if (!match) {
              sendJson(response, 404, { error: '文档不存在' })
              return
            }
            const filePath = path.resolve(projectRoot, match.split(' | ').slice(2).join(' | '))
            try {
              await access(filePath)
            } catch {
              sendJson(response, 404, { error: '源文件已被移动或删除' })
              return
            }
            if (process.platform !== 'win32') {
              sendJson(response, 501, { error: '当前平台暂不支持调用系统默认程序打开文件' })
              return
            }
            await execFileAsync('rundll32.exe', ['url.dll,FileProtocolHandler', filePath], { windowsHide: true })
            sendJson(response, 200, { opened: true })
            return
          }

          if (url === '/api/chat' && request.method === 'POST') {
            const body = await readJson(request)
            const question = String(body.question ?? '').trim()
            const baseUrl = String(body.baseUrl ?? 'https://api.deepseek.com/v1').replace(/\/$/, '')
            const model = String(body.model ?? 'deepseek-chat')
            const apiKeyEnv = String(body.apiKeyEnv ?? 'LOCALRAG_API_KEY')
            const apiKey = String(body.apiKey ?? '').trim() || process.env[apiKeyEnv] || ''
            if (!question) {
              sendJson(response, 400, { error: '问题不能为空' })
              return
            }
            if (!apiKey) {
              sendJson(response, 400, { error: '未配置 API Key，请在设置中填写或设置环境变量' })
              return
            }
            const result = await runLocalRag(['ask', question], {
              LOCALRAG_API_KEY: apiKey,
              LOCALRAG_BASE_URL: baseUrl,
              LOCALRAG_CHAT_MODEL: model,
            })
            sendJson(response, 200, { answer: result.stdout.trim(), stats: parseStats((await runLocalRag(['stats'])).stdout) })
            return
          }

          next()
        } catch (error) {
          const message = error instanceof Error ? error.message : '本地服务发生未知错误'
          sendJson(response, 500, { error: message })
        }
      })
    },
  }
}

export default defineConfig({
  plugins: [react(), localRagApi()],
  server: { port: 4173, host: '127.0.0.1' },
})
