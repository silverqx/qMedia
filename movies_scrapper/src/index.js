// const csfd = require('csfd-api')
const { csfd } = require('node-csfd-api')

const CSFD_SEARCH_MOVIE_DETAIL = 'csfd-search'
const CSFD_GET_MOVIE_DETAIL    = 'csfd-get'
const IMDB_SEARCH_MOVIE_DETAIL = 'imdb-search'

/**
 * Number of items to slice from search result.
 *
 * @type {number}
 */
const SEARCH_RESULT_SLICE = 15

// Fire it up
return main()

/**
 * Main function.
 *
 * Code is designed to search and return movie detail in one shot, it is better for
 * performance.
 *
 * @return {number}
 */
async function main() {
    // Validations
    if (process.argv.length < 4)
        return badParameters()

    // Prepare data
    const movieDatabase = process.argv[2]
    const testData = containsParam(['-t', '--test'])

    switch (movieDatabase) {
        case CSFD_SEARCH_MOVIE_DETAIL: {
            const searchQueryString = process.argv[3]
            if (testData)
                output(require('./testData').dataSearch)
            else
                output(await searchCsfdMovieDetail(searchQueryString))
            break
        }
        case CSFD_GET_MOVIE_DETAIL: {
            const filmId = parseInt(process.argv[3])
            if (testData)
                output(require('./testData').dataGet)
            else {
                if (isNaN(filmId))
                    throw new Error("Film id have to be number.")

                output(await getCsfdMovieDetail(filmId))
            }
            break
        }
        case IMDB_SEARCH_MOVIE_DETAIL: {
            const searchQueryString = process.argv[3]
            output(await searchImdbMovieDetail(searchQueryString))
            break
        }
        default:
            return badParameters()
    }

    return 0
}

/**
 * @param {string} searchQueryString
 * @return {Promise<{search: Object[], detail: Object}>}
 */
async function searchCsfdMovieDetail(searchQueryString) {
    const searchResult = await csfd.search(searchQueryString)
    return {
        search: searchResult.movies.slice(0, SEARCH_RESULT_SLICE),
        detail: await csfd.movie(searchResult.movies[0].id),
    }
}

/**
 * @param {number} filmId
 * @return {Promise<Object>}
 */
async function getCsfdMovieDetail(filmId) {
    return await csfd.movie(filmId)
}

async function searchImdbMovieDetail(searchQueryString) {
    return {}
}

/**
 * Bad parameters passed on the command line.
 *
 * @return {number}
 */
function badParameters() {
    console.error(new Error('Bad parameters format'))
    process.exitCode = 1
    return 1
}

/**
 * Contains process.argv this parameter?
 *
 * @param {string|string[]} param Command line parameter to search.
 * @return {boolean}
 */
function containsParam(param) {
    const parameter = typeof param === 'string' ? [param] : param
    return -1 !== process.argv.slice(2)
        .findIndex(value =>
            (-1 !== parameter.findIndex(paramVal => value.includes(paramVal))))
}

/**
 * Output result to the console.
 *
 * @param {Object} data Data to output.
 */
function output(data) {
    if (containsParam(['-r', '--raw']))
        console.dir(data, {depth: 3})
    else
        console.log(JSON.stringify(data))
}
